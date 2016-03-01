
#include <base/string.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <dim-sum/mem.h>
#include <dim-sum/sched.h>

struct file * first_file;
int nr_files = 0;

extern unsigned long event;
static void insert_file_free(struct file *file)
{
	file->f_next = first_file;
	file->f_prev = first_file->f_prev;
	file->f_next->f_prev = file;
	file->f_prev->f_next = file;
	first_file = file;
}

static void remove_file_free(struct file *file)
{
	if (first_file == file)
		first_file = first_file->f_next;
	if (file->f_next)
		file->f_next->f_prev = file->f_prev;
	if (file->f_prev)
		file->f_prev->f_next = file->f_next;
	file->f_next = file->f_prev = NULL;
}

static void put_last_free(struct file *file)
{
	remove_file_free(file);
	file->f_prev = first_file->f_prev;
	file->f_prev->f_next = file;
	file->f_next = first_file;
	file->f_next->f_prev = file;
}

void grow_files(void)
{
	struct file * file;
	int i;

	file = (struct file *) dim_sum_pages_alloc(0, MEM_NORMAL);

	if (!file)
		return;

	nr_files+=i= PAGE_SIZE/sizeof(struct file);

	if (!first_file)
	{
		file->f_next = file->f_prev = first_file = file;
		file++;
		i--;
	}
	
	for (; i ; i--)
	{
		insert_file_free(file);
		file++;
	}
}

/**
 * ����һ�����õ��ļ�������
 */
struct file * get_empty_filp(void)
{
	int i;
	struct file * f;

	if (!first_file)
		grow_files();
repeat:
	for (f = first_file, i=0; i < nr_files; i++, f = f->f_next)
		if (!atomic_read(&f->f_count)) {
			remove_file_free(f);
			memset(f,0,sizeof(*f));
			put_last_free(f);
			atomic_set(&f->f_count, 1);
			f->f_version = ++event;
			return f;
		}
	if (nr_files < NR_FILE) {
		grow_files();
		goto repeat;
	}
	return NULL;
}

/**
 * �ͷ��ļ��������������ŵ����ж�����
 */
static void __release_file(struct file *file)
{
}

void release_file(struct file *file)
{
	if (atomic_dec_and_test(&file->f_count)) {
		__release_file(file);
	}
}
struct files_struct globle_files_struct;
struct fs_struct globle_fs_struct;

/**
 * Ϊ�½��������ļ�ϵͳ����
 * parent��ǰ���̣���init������˵��parentΪnull
 * new�´����Ľ���
 */
int init_task_fs(struct task_struct *parent, struct task_struct *new)
{
	if (parent == NULL)
	{
		memset(&globle_files_struct, 0, sizeof(globle_files_struct));
		memset(&globle_fs_struct, 0, sizeof(globle_fs_struct));
		globle_files_struct.max_fds = __FD_SETSIZE;
		globle_files_struct.next_fd = 0;
		spin_lock_init(&globle_files_struct.file_lock);
		rwlock_init(&globle_fs_struct.lock);
	}
	new->files = &globle_files_struct;
	new->fs = &globle_fs_struct;

	return 0;
}

/**
 * ģ���ʼ��
 */
unsigned long file_table_init(void)
{
	first_file = NULL;

	return 0;
}


/**
 * ���ļ����ת��Ϊ�ļ��������������������������
 */
struct file *fd2file(unsigned int fd) {
	struct file *file = NULL;
	struct files_struct *files = current->files;

	if (fd >= NR_OPEN) {
		return file;
	}
	spin_lock(&files->file_lock);
	file = files->fd_array[fd];
	if (file) {
		hold_file(file);
	}
	spin_unlock(&files->file_lock);

	return file;
}