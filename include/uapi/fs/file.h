
#ifndef __PHOENIX_FILE_H
#define __PHOENIX_FILE_H

#include <asm/atomic.h>
#include <linux/posix_types.h>
#include <linux/compiler.h>
#include <dim-sum/spinlock.h>

#define NR_OPEN_DEFAULT 1024

/**
 * ���̵�ǰ�򿪵��ļ���
 */
struct files_struct {
		/**
		 * �����ñ�Ķ�д��������
		 */
        spinlock_t file_lock;     /* Protects all the below members.  Nests inside tsk->alloc_lock */
		/**
		 * �ļ�����ĵ�ǰ����š�
		 */
        int max_fds;
		/**
		 * �ļ��������ĵ�ǰ����š�
		 */
        int max_fdset;
		/**
		 * �ϴη��������ļ���������1.
		 */
        int next_fd;
		/**
		 * ���ļ���������ָ�롣
		 */
        fd_set *open_fds;
		/**
		 * �ļ�����ָ��ĳ�ʼ�����顣
		 */
        struct file * fd_array[__FD_SETSIZE];
};
		
#endif
