
#include <dim-sum/mem.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/msdos_fs.h>
#include <dim-sum/sched.h>
#include <asm/current.h>
#include <linux/time.h>
#include <linux/utime.h>
#include <dim-sum/errno.h>

extern int fcntl_getlk(unsigned int, struct flock *);
extern int fcntl_setlk(unsigned int, unsigned int, struct flock *);
extern int sock_fcntl (struct file *, unsigned int cmd, unsigned long arg);

/**
 * xby_debug, ׮����
 * Ŀǰ�ݲ�ʵ���ļ���
 */
int fcntl_getlk(unsigned int fd, struct flock *l)
{
	return 0;
}
int fcntl_setlk(unsigned int fd, unsigned int cmd, struct flock *l)
{
	return 0;
}


/**
 * xby_debug, ׮����
 * Ŀǰ�ݲ�ʵ��socket�Ŀ���
 */
int sock_fcntl (struct file *f, unsigned int cmd, unsigned long arg)
{
	return 0;
}


/**
 * �����ļ����
 */
static int dupfd(unsigned int fd, unsigned int arg)
{
	struct files_struct *files = current->files;

	//�������
	if (fd >= NR_OPEN)
		return -EBADF;
	if (arg >= NR_OPEN)
		return -EINVAL;

	//��ȡ�ļ���
	spin_lock(&files->file_lock);
	//�����ǰ�ļ���������ڣ����˳�
	if (!files->fd_array[fd]) {
		spin_unlock(&files->file_lock);
		return -EBADF;
	}

	if ((arg != 0) //Ŀ������ռ����
		&& files->fd_array[arg]) {
		spin_unlock(&files->file_lock);
		return -EMFILE;
	}
	//������һ�����þ��
	while (arg < NR_OPEN) {
		if (files->fd_array[arg])
			arg++;
		else
			break;
	}

	//�޿��þ����
	if (arg >= NR_OPEN) {
		spin_unlock(&files->file_lock);
		return -EMFILE;
	}
	//FD_CLR(arg, &current->files->close_on_exec);
	//�����ļ����������������ü���
	files->fd_array[arg] = files->fd_array[fd];
	hold_file(files->fd_array[fd]);
	//�ͷ���
	spin_unlock(&files->file_lock);
	return arg;
}

/**
 * dup2ϵͳ����
 */
asmlinkage int sys_dup2(unsigned int oldfd, unsigned int newfd)
{
	if (oldfd >= NR_OPEN)
		return -EBADF;
	if (newfd == oldfd)
		return newfd;

	if (newfd >= NR_OPEN)	
		return -EBADF;

	//���ȹر��¾��
	sys_close(newfd);
	//���ƾ��
	return dupfd(oldfd,newfd);
}

/**
 * dupϵͳ����
 */
asmlinkage int sys_dup(unsigned int fildes)
{
	return dupfd(fildes,0);
}

/**
 * fcntlϵͳ����
 */
asmlinkage int sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg)
{	
	struct file * filp;
	int ret = -EBADF;

	filp = fd2file(fd);
	if (!filp)
		return -EBADF;
	switch (cmd) {
		case F_DUPFD:
			ret = dupfd(fd,arg);
			break;
		case F_GETFD:
			ret = -EINVAL;
			break;
		case F_SETFD:
			if (arg&1)
				ret = -EINVAL;
			else
				ret = -EINVAL;
			break;
		case F_GETFL:
			ret = filp->f_flags;
			break;
		case F_SETFL://�����ļ�����
			// �������O_APPEND��־
			if (IS_APPEND(filp->f_inode) && !(arg & O_APPEND)) {
				ret = -EPERM;
				break;
			}

			//���FASYNC��־�����˱仯����֪ͨ�ļ�ϵͳ
			if ((arg & FASYNC) && !(filp->f_flags & FASYNC) &&
			    filp->f_op->fasync)
				filp->f_op->fasync(filp->f_inode, filp, 1);
			if (!(arg & FASYNC) && (filp->f_flags & FASYNC) &&
			    filp->f_op->fasync)
				filp->f_op->fasync(filp->f_inode, filp, 0);
			//ֻ������O_APPEND | O_NONBLOCK | FASYNC��������־
			filp->f_flags &= ~(O_APPEND | O_NONBLOCK | FASYNC);
			filp->f_flags |= arg & (O_APPEND | O_NONBLOCK |
						FASYNC);
			ret = 0;
			break;
		case F_GETLK:
			ret = fcntl_getlk(fd, (struct flock *) arg);
			break;
		case F_SETLK:
			ret = fcntl_setlk(fd, cmd, (struct flock *) arg);
			break;
		case F_SETLKW:
			ret = fcntl_setlk(fd, cmd, (struct flock *) arg);
			break;
		case F_GETOWN:
			ret = filp->f_owner;
			break;
		case F_SETOWN:
			filp->f_owner = arg;
			if (S_ISSOCK (filp->f_inode->i_mode)) {
				ret = sock_fcntl (filp, F_SETOWN, arg);
				break;
			}
			ret = 0;
			break;
		default:
			/* sockets need a few special fcntls. */
			if (S_ISSOCK (filp->f_inode->i_mode)) {
			     ret = (sock_fcntl (filp, cmd, arg));
				 break;
			}
			ret = -EINVAL;
	}

	put_file(filp);

	return ret;
}

/**
 * kill��tty��ص���������
 */
void kill_fasync(struct fasync_struct *fa, int sig)
{
	while (fa) {//������������
		if (fa->magic != FASYNC_MAGIC) {//��ȫ�Ծ��棬��ֹ�ڴ�����
			printk("kill_fasync: bad magic number in "
			       "fasync_struct!\n");
			return;
		}
		//�����ɱ�������������
		if (fa->fa_file->f_owner > 0)
			kill_proc(fa->fa_file->f_owner, sig, 1);
		else
			kill_pg(-fa->fa_file->f_owner, sig, 1);
		fa = fa->fa_next;
	}
}

