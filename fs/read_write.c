
#include <base/string.h>
#include <dim-sum/mem.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/msdos_fs.h>
#include <dim-sum/sched.h>
#include <asm/current.h>
#include <dim-sum/errno.h>

/**
 * readdirϵͳ����
 */
asmlinkage int sys_readdir(unsigned int fd, struct dirent * dirent, unsigned int count)
{
	struct file * file;
	struct inode * inode;
	int ret = -EBADF; 

	//���ļ����ת��Ϊ�ļ��������������������
	file = fd2file(fd);
	if (!file)
		return -EBADF;

	//�������
	if (!(inode = file->f_inode))
		goto out;

	ret = -ENOTDIR;
	if (file->f_op && file->f_op->readdir) {
		int size = count * sizeof(struct dirent);

		//��֤�������������Ч��
		ret = verify_area(VERIFY_WRITE, dirent, size);

		//�����ļ�ϵͳ��readdir�ص�����ȡĿ¼�е��ļ�
		if (!ret)
			ret = file->f_op->readdir(inode,file,dirent,count);
	}

out:
	put_file(file);//�ͷ��ļ�������������
	return ret;

}

/**
 * lseekϵͳ����
 */
asmlinkage int sys_lseek(unsigned int fd, off_t offset, unsigned int origin)
{
	struct file * file;
	int tmp = -1;
	int ret = -EBADF; 

	//���ļ����ת��Ϊ�ļ��������������������
	file = fd2file(fd);
	if (!file)
		return -EBADF;

	//�������
	if (!(file->f_inode))
		goto out;

	ret = -EINVAL;
	if (origin > 2)
		goto out;
	if (file->f_op && file->f_op->lseek) {
		ret = file->f_op->lseek(file->f_inode,file,offset,origin);
		goto out;
	}
	
	//�����дλ��
	switch (origin) {
		case 0:
			tmp = offset;
			break;
		case 1:
			tmp = file->f_pos + offset;
			break;
		case 2:
			if (!file->f_inode)
				return -EINVAL;
			tmp = file->f_inode->i_size + offset;
			break;
	}
	if (tmp < 0) {
		ret = -EINVAL;
		goto out;
	}

	//����λ��
	if (tmp != file->f_pos) {
		file->f_pos = tmp;
		file->f_reada = 0;//����Ԥ��λ��
		file->f_version = ++event;
	}
	ret = file->f_pos;

out:
	put_file(file);//�ͷ��ļ�������������
	return ret;
}

/**
 * llseekϵͳ����
 */
asmlinkage int sys_llseek(unsigned int fd, unsigned long offset_high,
			  unsigned long offset_low, loff_t * result,
			  unsigned int origin)
{
	struct file * file;
	loff_t tmp = -1;
	loff_t offset;
	int ret = -EBADF;

	//���ļ����ת��Ϊ�ļ��������������������
	file = fd2file(fd);
	if (!file)
		return -EBADF;
	//�������
	if (!(file->f_inode))
		goto out;

	ret = -EINVAL;
	if (origin > 2)
		goto out;
	
	if ((ret = verify_area(VERIFY_WRITE, result, sizeof(loff_t))))
		goto out;
	
	offset = (loff_t) (((unsigned long long) offset_high << 32) | offset_low);
	//������λ��
	switch (origin) {
		case 0:
			tmp = offset;
			break;
		case 1:
			tmp = file->f_pos + offset;
			break;
		case 2:
			//���ﲻ�����ж�f_inode����Ч����
			tmp = file->f_inode->i_size + offset;
			break;
	}
	if (tmp < 0) {
		ret = -EINVAL;
		goto out;
	}

	//���ö�дλ��
	file->f_pos = tmp;
	//����Ԥ��λ��
	file->f_reada = 0;
	file->f_version = ++event;
	memcpy_tofs(result, &file->f_pos, sizeof(loff_t));

	ret = 0;

out:
	put_file(file);//�ͷ��ļ�������������
	return ret;
}

/**
 * ���ļ�ϵͳ����
 */
asmlinkage int sys_read(unsigned int fd,char * buf,unsigned int count)
{
	struct file * file;
	struct inode * inode;
	int ret = -EBADF;

	//���ļ����ת��Ϊ�ļ��������������������
	file = fd2file(fd);
	if (!file)
		return -EBADF;

	//�������
	if (!(inode = file->f_inode))
		goto out;
	if (!(file->f_mode & 1))
		goto out;
	if (!file->f_op || !file->f_op->read)
		goto out;

	ret = 0;
	if (!count)
		goto out;

	//��������Ч����֤����ʱ����Ҫ��֤
	ret = verify_area(VERIFY_WRITE,buf,count);
	if (ret)
		goto out;
	//ֱ�ӵ����ļ�ϵͳ��read�ص�����ȡ���ݵ��������С�
	ret = file->f_op->read(inode,file,buf,count);

out:
	put_file(file);//�ͷ��ļ�������������
	return ret;
}

/**
 * д�ļ�ϵͳ����
 */
asmlinkage int sys_write(unsigned int fd,const char * buf,unsigned int count)
{
	struct file * file;
	struct inode * inode;
	int ret = -EBADF;

	int written;

	//���ļ����ת��Ϊ�ļ��������������������
	file = fd2file(fd);
	if (!file)
		return -EBADF;

	//�������
	if (!(inode = file->f_inode))
		goto out;
	if (!(file->f_mode & 2))
		goto out;
	if (!file->f_op || !file->f_op->write)
		goto out;

	ret = 0;
	if (!count)
		goto out;

	//��������Ч����֤����ʱ����Ҫ��֤
	ret = verify_area(VERIFY_READ,buf,count);
	if (ret)
		goto out;

	//ֱ�ӵ����ļ�ϵͳ��write�ص���д���ݵ��������С�
	written = file->f_op->write(inode,file,buf,count);
	//�����Ѿ����޸ģ�������������S_ISUID��S_ISGID��־��
	if (written > 0 && !suser() && (inode->i_mode & (S_ISUID | S_ISGID))) {
		struct iattr newattrs;
		newattrs.ia_mode = inode->i_mode & ~(S_ISUID | S_ISGID);
		newattrs.ia_valid = ATTR_MODE;
		//�޸Ľڵ�����
		notify_change(inode, &newattrs);
	}
	return written;

out:
	put_file(file);//�ͷ��ļ�������������
	return ret;
}
