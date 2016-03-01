
#include <dim-sum/mem.h>
#include <fs/fs.h>
#include <fs/mount.h>
#include <fs/fcntl.h>
#include <fs/msdos_fs.h>
#include <dim-sum/sched.h>
#include <dim-sum/errno.h>
#include <asm/current.h>

#define ACC_MODE(x) ("\000\004\002\006"[(x)&O_ACCMODE])

/**
 * �жϵ�ǰ�û��Ƿ���Ȩ�ް�ָ����ʽ�����ļ�
 */
int permission(struct inode * inode,int mask)
{
	return 0;
}

/**
 * ����û�������ļ���������
 * ע��:���ں�̬Ӧ�ò���Ҫ����ĸ��ƹ���
 */
int getname(const char * filename, char **result)
{
	//�ں�̬�У�����Ҫ�����ļ�������
	*result = (char *)filename;
	return 0;
}

/**
 * �ͷ��ں��ļ���������
 * ע��:���ں�̬Ӧ�ò���Ҫ������ͷŹ���
 */
void putname(char * name)
{
}

/**
 * ���������ڵ����ָ���ķ�������
 */
int follow_link(struct inode * dir, struct inode * inode,
	int flag, int mode, struct inode ** res_inode)
{
	if (!dir || !inode) {//�������
		iput(dir);
		iput(inode);
		*res_inode = NULL;
		return -ENOENT;
	}
	//���������ļ����˳�
	if (!inode->i_op || !inode->i_op->follow_link) {
		iput(dir);
		*res_inode = inode;
		return 0;
	}
	/**
	 * ���������ļ�
	 * xie.baoyouע��:����û�д���ݹ飬���ؾ��棬�޸�!!!!!!
	 */
	return inode->i_op->follow_link(dir,inode,flag,mode,res_inode);
}

/**
 * ��Ŀ¼�в���ָ���ļ��Ľڵ㡣
 * ����Ҫ���þ����ļ�ϵͳ�ķ�����ͬʱҲ��һЩ����ļ�顣
 * 		result:���ҵ����ļ��ڵ�
 */
int lookup(struct inode * dir,const char * name, int len,
	struct inode ** result)
{
	struct super_block * sb;

	//�������
	*result = NULL;
	if (!dir)
		return -ENOENT;
	//�����ϼ�Ŀ¼
	if (len==2 && name[0] == '.' && name[1] == '.') {
		if (dir == current->fs->root) {//�Ѿ��Ǹ�Ŀ¼�ˣ����������ϻ���
			*result = dir;
			return 0;
		} else if ((sb = dir->i_sb) && (dir == sb->s_mounted)) {//��ǰ�ڵ�����ļ�ϵͳ
			sb = dir->i_sb;
			iput(dir);
			/**
			 * ת�����󶨵ĸ��ڵ�
			 * ע��:������Ҫ�ص��޸ģ��г�ͻ!!!!!!
			 */
			dir = sb->s_covered;
			if (!dir)
				return -ENOENT;
			atomic_inc(&dir->i_count);
		}
	}
	//�ڵ�û��ʵ��lookup�ص���˵��������һ��Ŀ¼
	if (!dir->i_op || !dir->i_op->lookup) {
		iput(dir);
		return -ENOTDIR;
	}
	//���ҹ��̽���
	if (!len) {
		*result = dir;
		return 0;
	}
	//���ļ�ϵͳ�������lookup����
	return dir->i_op->lookup(dir,name,len,result);
}


/**
 * �����ļ��������������ڵ�Ŀ¼���Լ�����Ŀ¼�е�����
 */
static int dir_namei(const char * pathname, int * namelen, const char ** name,
	struct inode * base, struct inode ** res_inode)
{
	char c;
	const char * thisname;
	int len,error;
	struct inode * inode;
	struct fs_struct *fs = current->fs;

	*res_inode = NULL;
	read_lock(&fs->lock);
	if ((c = *pathname) == '/') {//�Ӹ�Ŀ¼��ʼ����
		iput(base);
		base = fs->root;
		pathname++;
		//����Ŀ¼��mount��������ü�������ֹĿ¼������ɾ����Ҳ��ֹ�ļ�ϵͳ��ж�ء�
		//atomic_inc(&fs->rootmnt->mnt_count);
		atomic_inc(&base->i_count);
	}
	else if (!base) {//û��ָ����Ŀ¼��������ǰĿ¼��ʼ����
		base = fs->pwd;
		//atomic_inc(&fs->pwdmnt->mnt_count);
		atomic_inc(&base->i_count);
	}
	read_unlock(&fs->lock);

	while (1) {
		thisname = pathname;
		//��������'/'
		for(len = 0; (c = *(pathname++))&&(c != '/'); len++)
			/* nothing */ ;
		if (!c)//������ϣ��˳�
			break;
		//����Ŀ¼���ü���
		atomic_inc(&base->i_count);
		//��baseĿ¼�в����ļ�
		error = lookup(base,thisname,len,&inode);
		if (error) {//����ʧ�ܣ��˳�
			iput(base);
			return error;
		}
		//�����������
		error = follow_link(base,inode,0,0,&base);
		if (error)
			return error;
	}
	//��������Ŀ¼��������Ŀ¼
	if (!base->i_op || !base->i_op->lookup) {
		iput(base);
		return -ENOTDIR;
	}
	//������������Ŀ¼���ļ���
	*name = thisname;
	*namelen = len;
	*res_inode = base;
	return 0;
}


static int _namei(const char * pathname, struct inode * base,
	int follow_links, struct inode ** res_inode)
{
	const char * basename;
	int namelen,error;
	struct inode * inode;

	*res_inode = NULL;
	error = dir_namei(pathname,&namelen,&basename,base,&base);
	if (error)
		return error;
	atomic_inc(&base->i_count);	/* lookup uses up base */
	error = lookup(base,basename,namelen,&inode);
	if (error) {
		iput(base);
		return error;
	}
	if (follow_links) {
		error = follow_link(base,inode,0,0,&inode);
		if (error)
			return error;
	} else
		iput(base);
	*res_inode = inode;
	return 0;
}

/*
 *	namei()
 *
 * is used by most simple commands to get the inode of a specified name.
 * Open, link etc use their own routines, but this is enough for things
 * like 'chmod' etc.
 */
int namei(const char * pathname, struct inode ** res_inode)
{
	int error;
	char * tmp;

	error = getname(pathname,&tmp);
	if (!error) {
		error = _namei(tmp,NULL,1,res_inode);
		putname(tmp);
	}
	return error;
}


/**
 * ����ļ���д����Ȩ��
 */
int get_write_access(struct inode * inode)
{
	inode->i_wcount++;
	return 0;
}

/**
 * �ͷ��ļ���д����Ȩ��
 */
void put_write_access(struct inode * inode)
{
	inode->i_wcount--;
}

int lnamei(const char * pathname, struct inode ** res_inode)
{
	int error;
	char * tmp;

	error = getname(pathname,&tmp);
	if (!error) {
		error = _namei(tmp,NULL,0,res_inode);
		putname(tmp);
	}
	return error;
}

/**
 * �����ļ������ҵ��ļ���Ӧ��inode������sys_openϵͳ����
 * ����sys_open���õ���Ҫ�������
 * ע��!�����flag��־��sys_openϵͳ���ò�ͬ:
 *	 	00 - no permissions needed
 *		01 - read permission needed
 *		10 - write permission needed
 *		11 - read/write permissions needed
 */
int open_namei(const char * pathname, int flag, int mode,
	struct inode ** res_inode, struct inode * base)
{
	const char * basename;
	int namelen,error;
	struct inode * dir, *inode;

	mode &= S_IALLUGO & ~current->fs->umask;
	mode |= S_IFREG;
	//�����ļ����ڵ�Ŀ¼��������Ŀ¼�е�����
	error = dir_namei(pathname,&namelen,&basename,base,&dir);
	if (error)
		return error;
	//���һ����Ŀ¼
	if (!namelen) {			/* special case: '/usr/' etc */
		if (flag & 2) {//��ͼдĿ¼
			iput(dir);
			return -EISDIR;
		}
		//�ж϶�Ŀ¼�Ĳ����Ƿ�Ϸ�
		if ((error = permission(dir,ACC_MODE(flag))) != 0) {
			iput(dir);
			return error;
		}
		//����Ŀ¼�ڵ�
		*res_inode=dir;
		return 0;
	}
	//���Ŀ¼�����ü���
	atomic_inc(&dir->i_count);	
	if (flag & O_CREAT) {//�����ļ�
		//��ýڵ���ź���
		down(&dir->i_sem);
		//�ٴβ����ļ��Ƿ���ڣ���Ϊ�����ط������Ѿ�������ͬ���ļ�
		error = lookup(dir,basename,namelen,&inode);
		if (!error) {//�ļ��Ѿ�����
			if (flag & O_EXCL) {//������Ҫ��ȷ���ļ��������������ǲ�û�д����ļ�
				iput(inode);
				error = -EEXIST;
			}
		} else if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0)//û�в���Ŀ¼��Ȩ��
			;	/* error is already set! */
		else if (!dir->i_op || !dir->i_op->create)//�ļ�ϵͳ��֧�ִ����ļ�
			error = -EACCES;
		else if (IS_RDONLY(dir))//ֻ��Ŀ¼
			error = -EROFS;
		else {
			//�����ļ�ϵͳ�Ļص�������һ���ļ��ڵ�
			atomic_inc(&dir->i_count);		/* create eats the dir */
			error = dir->i_op->create(dir,basename,namelen,mode,res_inode);
			up(&dir->i_sem);//�ͷ��ź���
			iput(dir);
			return error;
		}
		up(&dir->i_sem);//�ͷ��ź���
	} else {//�����򿪶��������ļ� 
		error = lookup(dir,basename,namelen,&inode);
	}
	
	if (error) {
		iput(dir);
		return error;
	}
	//���������ӣ���������ָ���Ŀ���ļ�
	error = follow_link(dir,inode,flag,mode,&inode);
	if (error)
		return error;
	//Ŀ¼�ǲ��ܱ�д��
	if (S_ISDIR(inode->i_mode) && (flag & 2)) {
		iput(inode);
		return -EISDIR;
	}
	//�ж�Ȩ��
	if ((error = permission(inode,ACC_MODE(flag))) != 0) {
		iput(inode);
		return error;
	}
	if (S_ISBLK(inode->i_mode) || S_ISCHR(inode->i_mode)) {//�豸�ļ�
		if (IS_NODEV(inode)) {//���������
			iput(inode);
			return -EACCES;
		}
		//�豸�ļ��������ض�
		flag &= ~O_TRUNC;
	} else {
		//ֻ���ڵ㣬������д
		if (IS_RDONLY(inode) && (flag & 2)) {
			iput(inode);
			return -EROFS;
		}
	}
	/*
	 * �ڵ�ֻ�ܱ��������
	 */
	if (IS_APPEND(inode) && ((flag & 2) && !(flag & O_APPEND))) {
		iput(inode);
		return -EPERM;
	}
	if (flag & O_TRUNC) {//�������
		struct iattr newattrs;

		if ((error = get_write_access(inode))) {
			iput(inode);
			return error;
		}
		newattrs.ia_size = 0;
		newattrs.ia_valid = ATTR_SIZE;
		//�����ļ�ϵͳ�Ļص�������ڵ㳤�ȱ仯
		if ((error = notify_change(inode, &newattrs))) {
			put_write_access(inode);
			iput(inode);
			return error;
		}
		inode->i_size = 0;
		//�����ļ�ϵͳ�Ļص����ض��ļ�
		if (inode->i_op && inode->i_op->truncate)
			inode->i_op->truncate(inode);
		//�����ļ�����
		inode->i_dirt = 1;
		put_write_access(inode);
	}
	*res_inode = inode;
	return 0;
}


static int do_link(struct inode * oldinode, const char * newname)
{
	struct inode * dir;
	const char * basename;
	int namelen, error;

	error = dir_namei(newname,&namelen,&basename,NULL,&dir);
	if (error) {
		iput(oldinode);
		return error;
	}
	if (!namelen) {
		iput(oldinode);
		iput(dir);
		return -EPERM;
	}
	if (IS_RDONLY(dir)) {
		iput(oldinode);
		iput(dir);
		return -EROFS;
	}
	if (dir->i_dev != oldinode->i_dev) {
		iput(dir);
		iput(oldinode);
		return -EXDEV;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		iput(oldinode);
		return error;
	}
	/*
	 * A link to an append-only or immutable file cannot be created
	 */
	if (IS_APPEND(oldinode) || IS_IMMUTABLE(oldinode)) {
		iput(dir);
		iput(oldinode);
		return -EPERM;
	}
	if (!dir->i_op || !dir->i_op->link) {
		iput(dir);
		iput(oldinode);
		return -EPERM;
	}
	atomic_inc(&dir->i_count);
	down(&dir->i_sem);
	error = dir->i_op->link(oldinode, dir, basename, namelen);
	up(&dir->i_sem);
	iput(dir);
	return error;
}

asmlinkage int sys_link(const char * oldname, const char * newname)
{
	int error;
	char * to;
	struct inode * oldinode;

	error = namei(oldname, &oldinode);
	if (error)
		return error;
	error = getname(newname,&to);
	if (error) {
		iput(oldinode);
		return error;
	}
	error = do_link(oldinode,to);
	putname(to);
	return error;
}


int do_mknod(const char * filename, int mode, dev_t dev)
{
	const char * basename;
	int namelen, error;
	struct inode * dir;

	mode &= ~current->fs->umask;
	error = dir_namei(filename,&namelen,&basename, NULL, &dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		return error;
	}
	if (!dir->i_op || !dir->i_op->mknod) {
		iput(dir);
		return -EPERM;
	}
	atomic_inc(&dir->i_count);
	down(&dir->i_sem);
	error = dir->i_op->mknod(dir,basename,namelen,mode,dev);
	up(&dir->i_sem);
	iput(dir);
	return error;
}

asmlinkage int sys_mknod(const char * filename, int mode, dev_t dev)
{
	int error;
	char * tmp;

#ifdef xbyd_debug
	if (S_ISDIR(mode) || (!S_ISFIFO(mode) && !fsuser()))
		return -EPERM;
#else
if (S_ISDIR(mode))
	return -EPERM;
#endif
	switch (mode & S_IFMT) {
	case 0:
		mode |= S_IFREG;
		break;
	case S_IFREG: case S_IFCHR: case S_IFBLK: case S_IFIFO: case S_IFSOCK:
		break;
	default:
		return -EINVAL;
	}
	error = getname(filename,&tmp);
	if (!error) {
		error = do_mknod(tmp,mode,dev);
		putname(tmp);
	}
	return error;
}

static int do_mkdir(const char * pathname, int mode)
{
	const char * basename;
	int namelen, error;
	struct inode * dir;

	error = dir_namei(pathname,&namelen,&basename,NULL,&dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		return error;
	}
	if (!dir->i_op || !dir->i_op->mkdir) {
		iput(dir);
		return -EPERM;
	}
	atomic_inc(&dir->i_count);
	down(&dir->i_sem);
	error = dir->i_op->mkdir(dir, basename, namelen, mode & 0777 & ~current->fs->umask);
	up(&dir->i_sem);
	iput(dir);
	return error;
}

asmlinkage int sys_mkdir(const char * pathname, int mode)
{
	int error;
	char * tmp;

	error = getname(pathname,&tmp);
	if (!error) {
		error = do_mkdir(tmp,mode);
		putname(tmp);
	}
	return error;
}


static int do_rename(const char * oldname, const char * newname)
{
	struct inode * old_dir, * new_dir;
	const char * old_base, * new_base;
	int old_len, new_len, error;

	error = dir_namei(oldname,&old_len,&old_base,NULL,&old_dir);
	if (error)
		return error;
	if ((error = permission(old_dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(old_dir);
		return error;
	}
	if (!old_len || (old_base[0] == '.' &&
	    (old_len == 1 || (old_base[1] == '.' &&
	     old_len == 2)))) {
		iput(old_dir);
		return -EPERM;
	}
	error = dir_namei(newname,&new_len,&new_base,NULL,&new_dir);
	if (error) {
		iput(old_dir);
		return error;
	}
	if ((error = permission(new_dir,MAY_WRITE | MAY_EXEC)) != 0){
		iput(old_dir);
		iput(new_dir);
		return error;
	}
	if (!new_len || (new_base[0] == '.' &&
	    (new_len == 1 || (new_base[1] == '.' &&
	     new_len == 2)))) {
		iput(old_dir);
		iput(new_dir);
		return -EPERM;
	}
	if (new_dir->i_dev != old_dir->i_dev) {
		iput(old_dir);
		iput(new_dir);
		return -EXDEV;
	}
	if (IS_RDONLY(new_dir) || IS_RDONLY(old_dir)) {
		iput(old_dir);
		iput(new_dir);
		return -EROFS;
	}
	/*
	 * A file cannot be removed from an append-only directory
	 */
	if (IS_APPEND(old_dir)) {
		iput(old_dir);
		iput(new_dir);
		return -EPERM;
	}
	if (!old_dir->i_op || !old_dir->i_op->rename) {
		iput(old_dir);
		iput(new_dir);
		return -EPERM;
	}
	atomic_inc(&new_dir->i_count);
	down(&new_dir->i_sem);
	error = old_dir->i_op->rename(old_dir, old_base, old_len, 
		new_dir, new_base, new_len);
	up(&new_dir->i_sem);
	iput(new_dir);
	return error;
}

asmlinkage int sys_rename(const char * oldname, const char * newname)
{
	int error;
	char * from, * to;

	error = getname(oldname,&from);
	if (!error) {
		error = getname(newname,&to);
		if (!error) {
			error = do_rename(from,to);
			putname(to);
		}
		putname(from);
	}
	return error;
}


static int do_rmdir(const char * name)
{
	const char * basename;
	int namelen, error;
	struct inode * dir;

	error = dir_namei(name,&namelen,&basename,NULL,&dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		return error;
	}
	/*
	 * A subdirectory cannot be removed from an append-only directory
	 */
	if (IS_APPEND(dir)) {
		iput(dir);
		return -EPERM;
	}
	if (!dir->i_op || !dir->i_op->rmdir) {
		iput(dir);
		return -EPERM;
	}
	return dir->i_op->rmdir(dir,basename,namelen);
}

asmlinkage int sys_rmdir(const char * pathname)
{
	int error;
	char * tmp;

	error = getname(pathname,&tmp);
	if (!error) {
		error = do_rmdir(tmp);
		putname(tmp);
	}
	return error;
}


static int do_symlink(const char * oldname, const char * newname)
{
	struct inode * dir;
	const char * basename;
	int namelen, error;

	error = dir_namei(newname,&namelen,&basename,NULL,&dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		return error;
	}
	if (!dir->i_op || !dir->i_op->symlink) {
		iput(dir);
		return -EPERM;
	}
	atomic_inc(&dir->i_count);
	down(&dir->i_sem);
	error = dir->i_op->symlink(dir,basename,namelen,oldname);
	up(&dir->i_sem);
	iput(dir);
	return error;
}

asmlinkage int sys_symlink(const char * oldname, const char * newname)
{
	int error;
	char * from, * to;

	error = getname(oldname,&from);
	if (!error) {
		error = getname(newname,&to);
		if (!error) {
			error = do_symlink(from,to);
			putname(to);
		}
		putname(from);
	}
	return error;
}


static int do_unlink(const char * name)
{
	const char * basename;
	int namelen, error;
	struct inode * dir;

	error = dir_namei(name,&namelen,&basename,NULL,&dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -EPERM;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		return error;
	}
	/*
	 * A file cannot be removed from an append-only directory
	 */
	if (IS_APPEND(dir)) {
		iput(dir);
		return -EPERM;
	}
	if (!dir->i_op || !dir->i_op->unlink) {
		iput(dir);
		return -EPERM;
	}
	return dir->i_op->unlink(dir,basename,namelen);
}

asmlinkage int sys_unlink(const char * pathname)
{
	int error;
	char * tmp;

	error = getname(pathname,&tmp);
	if (!error) {
		error = do_unlink(tmp);
		putname(tmp);
	}
	return error;
}

