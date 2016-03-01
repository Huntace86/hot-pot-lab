#ifndef __PHOENIX_SEMAPHORE_H
#define __PHOENIX_SEMAPHORE_H

#include <dim-sum/spinlock.h>

#if 0
/**
 * rcu�Ķ������򵥵Ľ�����ռ���ɡ�
 */
#define rcu_read_lock()		

/**
 * rcu_read_unlock - marks the end of an RCU read-side critical section.
 *
 * See rcu_read_lock() for more information.
 */
/**
 * �ͷ�rcu�Ķ������򵥵Ĵ���ռ���ɡ�
 */
#define rcu_read_unlock()	

#define read_seqbegin
#define read_seqretry

struct rw_semaphore {
	/**
	 * �������16λ�ļ�������
	 * ��16λ�Բ�����ʽ��ŵȴ�д�Ľ���������ע�����д�������Ĳ�ͬ����д�������������н���������˯�ߣ�������һ����ֵ����ʾд�ȴ������ˡ�
	 * ��16λ��ŷǵȴ��Ķ��ߺ�д��������
	 */
	signed long		count;
};

struct semaphore {
	atomic_t count;
	int sleepers;
};

#endif

#define SEM_ID unsigned long

#define SEM_NOT_GOT  0
#define SEM_GOT_SEM 1
#define SEM_DELETED 2

#define ERROR_SEMID_INVALID  1
#define ERROR_SEM_IN_INTR   2
#define ERROR_SEM_IS_UNREFED 3
#define ERROR_SEM_IS_DELETED 4
#define ERROR_SEM_TIMED_OUT  5

#define SEM_MAGIC 0x37210720

struct semb_struct;
struct semb_wait_item{
	unsigned int flag;
    struct semb_struct  *wait_sem;             /* ���ڵȴ���SEM  */
    struct task_struct  *task;
    struct list_head list;     /* �����ź����ĵȴ�����  */
};
struct semb_struct{
	unsigned long owner;
	int option;
    u32   magic;
	spinlock_t lock;
	struct list_head wait_list;
	atomic_t refcount;
	struct list_head all_list;
};

struct semc_struct;
struct semc_wait_item{
	unsigned int flag;
    struct semc_struct  *wait_sem;             /* ���ڵȴ���SEM  */
    struct task_struct  *task;
    struct list_head list;     /* �����ź����ĵȴ�����  */
};
struct semc_struct{
	int count;
	int option;
    u32   magic; 
	spinlock_t lock;
	struct list_head wait_list;
	atomic_t refcount;
	struct list_head all_list;
};


struct semm_struct;
struct semm_wait_item{
	unsigned int flag;
    struct semm_struct  *wait_sem;             /* ���ڵȴ���SEM  */
    struct task_struct  *task;
    struct list_head list;     /* �����ź����ĵȴ�����  */
};
struct semm_struct{
	unsigned long owner;
	int recurse;
	int option;
    	u32   magic;
	spinlock_t lock;
	struct list_head wait_list;
	atomic_t refcount;
	struct list_head all_list;
};

#ifndef SEM_Q_PRIOITY
#define SEM_Q_PRIOITY 1
#endif

#ifndef SEM_INVERSION_SAFE
#define SEM_INVERSION_SAFE 8
#endif

#define MSG_Q_ID unsigned long

#define MSGQ_WAIT 0
#define MSGQ_TRY_AGAIN 1
#define MSGQ_DELETED 2

#define MSGQ_MAGIC 0x37210709

#define MSGQ_Q_FIFO 0
#define MSGQ_Q_PRIOITY 1

#define ERROR_MSGQID_INVALID -1
#define ERROR_MSGQ_IN_INTR    -2
#define ERROR_MSGQ_IS_UNREFED -3
#define ERROR_MSGQ_IS_DELETED -4
#define ERROR_MSGQ_TIMED_OUT  -5
#define ERROR_MSG_LENS_OVER  -6
#define ERROR_BUF_LENS_UNDER  -7
#define ERROR_MSGQ_FULL       -8
#define ERROR_MSGQ_EMPTY      -9

struct msgq_struct;
struct msgq_wait_recv_item{
	unsigned int flag;
    struct msgq_struct  *wait_msgq;             /* ���ڵȴ���SEM  */
    struct task_struct  *task;
    struct list_head list;     /* �����ź����ĵȴ�����  */
};
struct msgq_wait_send_item{
	unsigned int flag;
    struct msgq_struct  *wait_msgq;             /* ���ڵȴ���SEM  */
    struct task_struct  *task;
    struct list_head list;     /* �����ź����ĵȴ�����  */
};
struct msgq_node{
	struct list_head list;
	char	*data;
	int		size;
};
	
struct msgq_struct{
	int option;
    u32  magic;
	int msg_size;
	int msg_num;
	char *data;
	char *nodes;
	int	msg_count;
	struct list_head msg_list;
	struct list_head free_list;
	spinlock_t lock;
	struct list_head wait_recv_list;
	struct list_head wait_send_list;
	atomic_t refcount;
	struct list_head all_list;
};


SEM_ID ker_semBCreate(int option);
int ker_semBDelete(SEM_ID semid);
int ker_semBTake(SEM_ID semid, int timeout);
int ker_semBGive(SEM_ID semid);


SEM_ID SysSemMCreate(unsigned int uiOption);
#define SysSemMTake(tSemId, iTimeOut) ker_semMTake(tSemId, iTimeOut)

int ker_semMGive(SEM_ID semid);
#define SysSemMGive(tSemId) ker_semMGive(tSemId)





SEM_ID ker_semCCreate(int option, signed long num);

int ker_semCDelete(SEM_ID semid);

int ker_semCTake(SEM_ID semid, int timeout);

int ker_semCGive(SEM_ID semid);


typedef struct semc_struct semaphore;
#define __SEMAPHORE_INITIALIZER(name) {				\
	}

#define DECLARE_SEMAPHORE(name) \
	semaphore name = __SEMAPHORE_INITIALIZER(name)

extern void init_semaphore(semaphore *sema);
extern void down(semaphore * sem);
extern void up(semaphore * sem);


SEM_ID ker_semMCreate(int option);

int ker_semMDelete(SEM_ID semid);

int ker_semMTake(SEM_ID semid, int timeout);

int ker_semMGive(SEM_ID semid);

int SysSemMDelete (SEM_ID tSemId);

MSG_Q_ID  ker_msgQCreate( int max_msgs, int msglen, int opt );

int ker_MsgQDelete(MSG_Q_ID msgqid);

int  ker_msgQReceive( MSG_Q_ID queue, char *msgbuf, unsigned int buflen,
                              int wait );

int  ker_msgQSend( MSG_Q_ID queue, char *msg, unsigned int msglen,
                           int wait, int pri );

int ker_msgQNumMsgs(MSG_Q_ID queue);

#endif
