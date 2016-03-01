#ifndef _LINUX_WAIT_H
#define _LINUX_WAIT_H

#include <linux/linkage.h>

//#include <dim-sum/sched.h>
#include <asm/current.h>
#include <dim-sum/spinlock.h>

#define WNOHANG		0x00000001
#define WUNTRACED	0x00000002
#define WSTOPPED	WUNTRACED
#define WEXITED		0x00000004
#define WCONTINUED	0x00000008
#define WNOWAIT		0x01000000	/* Don't reap, just poll status.  */

#define __WNOTHREAD	0x20000000	/* Don't wait on children of other threads in this group */
#define __WALL		0x40000000	/* Wait on all children, regardless of type */
#define __WCLONE	0x80000000	/* Wait only on non-SIGCHLD children */

/* First argument to waitid: */
#define P_ALL		0
#define P_PID		1
#define P_PGID		2


struct task_struct;
typedef struct __wait_queue wait_queue_t;
typedef int (*wait_queue_func_t)(wait_queue_t *wait, unsigned mode, int sync, void *key);
int default_wake_function(wait_queue_t *wait, unsigned mode, int sync, void *key);

/**
 * �ȴ������е�Ԫ�أ�ÿ��Ԫ�ش���һ��˯�ߵĽ��̡�
 * �ý��̵ȴ�ĳһ���¼��ķ�����
 */
struct __wait_queue {
	/**
	 * ���flagsΪ1,��ʾ�ȴ������ǻ���ġ��ȴ������ٽ���Դ�Ľ��̾��ǵ��͵Ļ�����̡�
	 * ���flagsΪ0����ʾ�ȴ������Ƿǻ���ġ��ȴ�����¼��Ľ����Ƿǻ���ġ�
	 */
	unsigned int flags;
#define WQ_FLAG_EXCLUSIVE	0x01
	/**
	 * ˯���ڶ����ϵĽ�����������
	 */
	struct task_struct * task;
	/**
	 * �ȴ������е�˯�߽����Ժ��ַ�ʽ���ѡ�
	 */
	wait_queue_func_t func;
	/**
	 * ͨ����ָ�뽫˯�߽�������������
	 */
	struct list_head task_list;
};


/**
 * �ȴ����е�ͷ
 */
struct __wait_queue_head {
	/**
	 * ���ڵȴ����п������жϴ��������ں˺����޸ģ����Ա����˫��������б���������������ͬʱ���ʡ�
	 * ��ͬ������lock�������ﵽ�ġ�
	 */
	spinlock_t lock;
	/**
	 * �ȴ����������ͷ��
	 */
	struct list_head task_list;
};
typedef struct __wait_queue_head wait_queue_head_t;

/*
 * Macros for declaration and initialisaton of the datatypes
 */

#define __WAITQUEUE_INITIALIZER(name, tsk) {				\
	.task		= tsk,						\
	.func		= default_wake_function,			\
	.task_list	= { NULL, NULL } }

#define DECLARE_WAITQUEUE(name, tsk)					\
	wait_queue_t name = __WAITQUEUE_INITIALIZER(name, tsk)

/**
 * ��ʼ��wait_queue_t�ṹ�ı���
 */
static inline void init_waitqueue_entry(wait_queue_t *q, struct task_struct *p)
{
	q->flags = 0;
	q->task = p;
	q->func = default_wake_function;
}

#define __WAIT_QUEUE_HEAD_INITIALIZER(name) {				\
	.lock		= SPIN_LOCK_UNLOCKED,				\
	.task_list	= { &(name).task_list, &(name).task_list } }

extern void __init_waitqueue_head(wait_queue_head_t *q, const char *name, struct lock_class_key *);

#define init_waitqueue_head(q)				\
	do {						\
		static struct lock_class_key __key;	\
							\
		__init_waitqueue_head((q), #q, &__key);	\
	} while (0)

/**
 * �ú궨��һ���µȴ����е�ͷ������̬������һ����name�ĵȴ����е�ͷ�������Ըñ�����lock��task_list�ֶν��г�ʼ��
 */
#define DECLARE_WAIT_QUEUE_HEAD(name) \
	wait_queue_head_t name = __WAIT_QUEUE_HEAD_INITIALIZER(name)
	

static inline int waitqueue_active(wait_queue_head_t *q)
{
	return !list_empty(&q->task_list);
}

static inline void __add_wait_queue(wait_queue_head_t *head, wait_queue_t *new)
{
	list_add(&new->task_list, &head->task_list);
}

static inline void __remove_wait_queue(wait_queue_head_t *head,
							wait_queue_t *old)
{
	list_del(&old->task_list);
}

extern void add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait);
extern void add_wait_queue_exclusive(wait_queue_head_t *q, wait_queue_t *wait);
extern void remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait);

//extern void FASTCALL(add_wait_queue(wait_queue_head_t *q, wait_queue_t * wait));
//extern void FASTCALL(remove_wait_queue(wait_queue_head_t *q, wait_queue_t * wait));
//void FASTCALL(__wake_up(wait_queue_head_t *q, unsigned int mode, int nr, void *key));

#define wake_up(x)			__wake_up(x, TASK_UNINTERRUPTIBLE | TASK_INTERRUPTIBLE, 1, NULL)
#define wake_up_nr(x, nr)		__wake_up(x, TASK_UNINTERRUPTIBLE | TASK_INTERRUPTIBLE, nr, NULL)
#define wake_up_all(x)			__wake_up(x, TASK_UNINTERRUPTIBLE | TASK_INTERRUPTIBLE, 0, NULL)
#define wake_up_interruptible(x)	__wake_up(x, TASK_INTERRUPTIBLE, 1, NULL)
#define wake_up_interruptible_nr(x, nr)	__wake_up(x, TASK_INTERRUPTIBLE, nr, NULL)
#define wake_up_interruptible_all(x)	__wake_up(x, TASK_INTERRUPTIBLE, 0, NULL)

void fastcall sleep_on(wait_queue_head_t *q);
extern void interruptible_sleep_on(wait_queue_head_t *p);

struct select_table_entry {
	wait_queue_t wait;
	wait_queue_head_t * wait_address;
};

typedef struct select_table_struct {
	int nr;
	struct select_table_entry * entry;
} select_table;


#endif
