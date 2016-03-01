
#include <base/string.h>
#include <fs/fs.h>
#include <asm/page.h>
#include <dim-sum/mem.h>
#include <dim-sum/sched.h>

unsigned long event = 0;


static struct inode_hash_entry {
	struct inode * inode;
	int updating;
} hash_table[NR_IHASH];

static struct inode * first_inode;
static DECLARE_WAIT_QUEUE_HEAD(inode_wait);
static int nr_inodes = 0, nr_free_inodes = 0;

static inline int const hashfn(dev_t dev, unsigned int i)
{
	return (dev ^ i) % NR_IHASH;
}

static inline struct inode_hash_entry * const hash(dev_t dev, int i)
{
	return hash_table + hashfn(dev, i);
}

static void insert_inode_free(struct inode *inode)
{
	inode->i_next = first_inode;
	inode->i_prev = first_inode->i_prev;
	inode->i_next->i_prev = inode;
	inode->i_prev->i_next = inode;
	first_inode = inode;
}

static void remove_inode_free(struct inode *inode)
{
	if (first_inode == inode)
		first_inode = first_inode->i_next;
	if (inode->i_next)
		inode->i_next->i_prev = inode->i_prev;
	if (inode->i_prev)
		inode->i_prev->i_next = inode->i_next;
	inode->i_next = inode->i_prev = NULL;
}

void insert_inode_hash(struct inode *inode)
{
	struct inode_hash_entry *h;
	h = hash(inode->i_dev, inode->i_ino);

	inode->i_hash_next = h->inode;
	inode->i_hash_prev = NULL;
	if (inode->i_hash_next)
		inode->i_hash_next->i_hash_prev = inode;
	h->inode = inode;
}

static void remove_inode_hash(struct inode *inode)
{
	struct inode_hash_entry *h;
	h = hash(inode->i_dev, inode->i_ino);

	if (h->inode == inode)
		h->inode = inode->i_hash_next;
	if (inode->i_hash_next)
		inode->i_hash_next->i_hash_prev = inode->i_hash_prev;
	if (inode->i_hash_prev)
		inode->i_hash_prev->i_hash_next = inode->i_hash_next;
	inode->i_hash_prev = inode->i_hash_next = NULL;
}

static void put_last_free(struct inode *inode)
{
	remove_inode_free(inode);
	inode->i_prev = first_inode->i_prev;
	inode->i_prev->i_next = inode;
	inode->i_next = first_inode;
	inode->i_next->i_prev = inode;
}

void grow_inodes(void)
{
	struct inode * inode;
	int i;

	if (!(inode = (struct inode*) dim_sum_pages_alloc(0, MEM_NORMAL)))
		return;

	i=PAGE_SIZE / sizeof(struct inode);
	nr_inodes += i;
	nr_free_inodes += i;

	if (!first_inode)
	{
		memset(inode, 0, sizeof(struct inode));
		init_waitqueue_head(&inode->i_wait);
		inode->i_next = inode->i_prev = first_inode = inode;
		inode++;
		i--;
	}
	
	for ( ; i ; i-- )
	{
		memset(inode, 0, sizeof(struct inode));
		init_waitqueue_head(&inode->i_wait);
		insert_inode_free(inode);
		inode++;
	}
}

unsigned long inode_init(void)
{
	memset(hash_table, 0, sizeof(hash_table));
	first_inode = NULL;

	return 0;
}

static void __wait_on_inode(struct inode *);

static inline void wait_on_inode(struct inode * inode)
{
	if (inode->i_lock)
		__wait_on_inode(inode);
}

static inline void lock_inode(struct inode * inode)
{
	wait_on_inode(inode);
	inode->i_lock = 1;
}

static inline void unlock_inode(struct inode * inode)
{
	inode->i_lock = 0;
	wake_up(&inode->i_wait);
}

/*
 * Note that we don't want to disturb any wait-queues when we discard
 * an inode.
 *
 * Argghh. Got bitten by a gcc problem with inlining: no way to tell
 * the compiler that the inline asm function 'memset' changes 'inode'.
 * I've been searching for the bug for days, and was getting desperate.
 * Finally looked at the assembler output... Grrr.
 *
 * The solution is the weird use of 'volatile'. Ho humm. Have to report
 * it to the gcc lists, and hope we can do this more cleanly some day..
 */
void clear_inode(struct inode * inode)
{
	//wait_queue_t * wait;

	wait_on_inode(inode);
	remove_inode_hash(inode);
	remove_inode_free(inode);
	//wait = ((volatile struct inode *) inode)->i_wait;
	if (atomic_read(&inode->i_count))
		nr_free_inodes++;
	memset(inode,0,sizeof(*inode));
	//((volatile struct inode *) inode)->i_wait = wait;
	insert_inode_free(inode);
}

int fs_may_mount(dev_t dev)
{
	struct inode * inode, * next;
	int i;

	next = first_inode;
	for (i = nr_inodes ; i > 0 ; i--) {
		inode = next;
		next = inode->i_next;	/* clear_inode() changes the queues.. */
		if (inode->i_dev != dev)
			continue;
		if (atomic_read(&inode->i_count) || inode->i_dirt || inode->i_lock)
			return 0;
		clear_inode(inode);
	}
	return 1;
}

int fs_may_umount(dev_t dev, struct inode * mount_root)
{
	struct inode * inode;
	int i;

	inode = first_inode;
	for (i=0 ; i < nr_inodes ; i++, inode = inode->i_next) {
		if (inode->i_dev != dev || !atomic_read(&inode->i_count))
			continue;
		if (inode == mount_root && atomic_read(&inode->i_count) == 1)
			continue;
		return 0;
	}
	return 1;
}

int fs_may_remount_ro(dev_t dev)
{
	struct file * file;
	int i;

	/* Check that no files are currently opened for writing. */
	for (file = first_file, i=0; i<nr_files; i++, file=file->f_next) {
		if (!atomic_read(&file->f_count) || !file->f_inode ||
		    file->f_inode->i_dev != dev)
			continue;
		if (S_ISREG(file->f_inode->i_mode) && (file->f_mode & 2))
			return 0;
	}
	return 1;
}
extern int xby_debug_flag;
static void write_inode(struct inode * inode)
{
	if (!inode->i_dirt)
		return;

	wait_on_inode(inode);
	if (!inode->i_dirt)
		return;
	if (!inode->i_sb || !inode->i_sb->s_op || !inode->i_sb->s_op->write_inode) {
		inode->i_dirt = 0;
		return;
	}
	inode->i_lock = 1;	
	inode->i_sb->s_op->write_inode(inode);
	unlock_inode(inode);
}

static void read_inode(struct inode * inode)
{
	lock_inode(inode);
	if (inode->i_sb && inode->i_sb->s_op && inode->i_sb->s_op->read_inode)
		inode->i_sb->s_op->read_inode(inode);
	unlock_inode(inode);
}

/* POSIX UID/GID verification for setting inode attributes */
int inode_change_ok(struct inode *inode, struct iattr *attr)
{
	return 0;
}

/*
 * Set the appropriate attributes from an attribute structure into
 * the inode structure.
 */
void inode_setattr(struct inode *inode, struct iattr *attr)
{
	if (attr->ia_valid & ATTR_UID)
		inode->i_uid = attr->ia_uid;
	if (attr->ia_valid & ATTR_GID)
		inode->i_gid = attr->ia_gid;
	if (attr->ia_valid & ATTR_SIZE)
		inode->i_size = attr->ia_size;
	if (attr->ia_valid & ATTR_ATIME)
		inode->i_atime = attr->ia_atime;
	if (attr->ia_valid & ATTR_MTIME)
		inode->i_mtime = attr->ia_mtime;
	if (attr->ia_valid & ATTR_CTIME)
		inode->i_ctime = attr->ia_ctime;
	if (attr->ia_valid & ATTR_MODE) {
		inode->i_mode = attr->ia_mode;
	}
	inode->i_dirt = 1;
}

/*
 * notify_change is called for inode-changing operations such as
 * chown, chmod, utime, and truncate.  It is guaranteed (unlike
 * write_inode) to be called from the context of the user requesting
 * the change.  It is not called for ordinary access-time updates.
 * NFS uses this to get the authentication correct.  -- jrs
 */

int notify_change(struct inode * inode, struct iattr *attr)
{
	int retval;

	if (inode->i_sb && inode->i_sb->s_op  &&
	    inode->i_sb->s_op->notify_change) 
		return inode->i_sb->s_op->notify_change(inode, attr);

	if ((retval = inode_change_ok(inode, attr)) != 0)
		return retval;

	inode_setattr(inode, attr);
	return 0;
}

/*
 * bmap is needed for demand-loading and paging: if this function
 * doesn't exist for a filesystem, then those things are impossible:
 * executables cannot be run from the filesystem etc...
 *
 * This isn't as bad as it sounds: the read-routines might still work,
 * so the filesystem would be otherwise ok (for example, you might have
 * a DOS filesystem, which doesn't lend itself to bmap very well, but
 * you could still transfer files to/from the filesystem)
 */
int bmap(struct inode * inode, int block)
{
	if (inode->i_op && inode->i_op->bmap)
		return inode->i_op->bmap(inode,block);
	return 0;
}

void invalidate_inodes(dev_t dev)
{
	struct inode * inode, * next;
	int i;

	next = first_inode;
	for(i = nr_inodes ; i > 0 ; i--) {
		inode = next;
		next = inode->i_next;		/* clear_inode() changes the queues.. */
		if (inode->i_dev != dev)
			continue;
		if (atomic_read(&inode->i_count) || inode->i_dirt || inode->i_lock) {
			printk("VFS: inode busy on removed device %d/%d\n", MAJOR(dev), MINOR(dev));
			continue;
		}
		clear_inode(inode);
	}
}

void sync_inodes(dev_t dev)
{
	int i;
	struct inode * inode;

	inode = first_inode;
	for(i = 0; i < nr_inodes*2; i++, inode = inode->i_next) {
		if (dev && inode->i_dev != dev)
			continue;
		wait_on_inode(inode);
		if (inode->i_dirt)
			write_inode(inode);
	}
}

void iput(struct inode * inode)
{
	if (!inode)
		return;
	wait_on_inode(inode);
	if (!atomic_read(&inode->i_count)) {
		printk("VFS: iput: trying to free free inode\n");
		printk("VFS: device %d/%d, inode %lu, mode=0%07o\n",
			MAJOR(inode->i_rdev), MINOR(inode->i_rdev),
					inode->i_ino, inode->i_mode);
		return;
	}
#ifdef xby_debug
	if (inode->i_pipe)
		wake_up_interruptible(&PIPE_WAIT(*inode));
#endif
	if (!atomic_dec_and_test(&inode->i_count)) {
		return;
	}

repeat:
	wake_up(&inode_wait);
#ifdef xby_debug
	if (inode->i_pipe) {
		unsigned long page = (unsigned long) PIPE_BASE(*inode);
		PIPE_BASE(*inode) = NULL;
		dim_sum_pages_free(page);
	}
#endif
	if (inode->i_sb && inode->i_sb->s_op && inode->i_sb->s_op->put_inode) {
		inode->i_sb->s_op->put_inode(inode);
		if (!inode->i_nlink)
			return;
	}
	if (inode->i_dirt) {
		write_inode(inode);	/* we can sleep - so do again */
		wait_on_inode(inode);
		goto repeat;
	}

	if (inode->i_mmap) {
		printk("iput: inode %lu on device %d/%d still has mappings.\n",
			inode->i_ino, MAJOR(inode->i_dev), MINOR(inode->i_dev));
		inode->i_mmap = NULL;
	}
	nr_free_inodes++;
	return;
}

struct inode * get_empty_inode(void)
{
	struct inode * inode, * best;
	int i;

	if (nr_inodes < NR_INODE && nr_free_inodes < (nr_inodes >> 2))
		grow_inodes();
repeat:
	inode = first_inode;
	best = NULL;
	for (i = 0; i<nr_inodes; inode = inode->i_next, i++) {
		if (!atomic_read(&inode->i_count)) {
			if (!best)
				best = inode;
			if (!inode->i_dirt && !inode->i_lock) {
				best = inode;
				break;
			}
		}
	}
	if (!best || best->i_dirt || best->i_lock)
		if (nr_inodes < NR_INODE) {
			grow_inodes();
			goto repeat;
		}
	inode = best;
	if (!inode) {
		printk("VFS: No free inodes - contact xie.baoyou\n");
		while (1);
		sleep_on(&inode_wait);
		goto repeat;
	}
	if (inode->i_lock) {
		wait_on_inode(inode);
		goto repeat;
	}
	if (inode->i_dirt) {
		write_inode(inode);
		goto repeat;
	}
	if (atomic_read(&inode->i_count))
		goto repeat;
	clear_inode(inode);
	atomic_set(&inode->i_count, 1);
	inode->i_nlink = 1;
	inode->i_version = ++event;
	init_semaphore(&inode->i_sem);
	init_waitqueue_head(&inode->i_wait);
	nr_free_inodes--;
	if (nr_free_inodes < 0) {
		printk ("VFS: get_empty_inode: bad free inode count.\n");
		nr_free_inodes = 0;
	}
	return inode;
}

struct inode * get_pipe_inode(void)
{
#ifdef xby_debug
	struct inode * inode;
	extern struct inode_operations pipe_inode_operations;

	if (!(inode = get_empty_inode()))
		return NULL;
	if (!(PIPE_BASE(*inode) = (char*) dim_sum_pages_alloc(0, MEM_NORMAL))) {
		iput(inode);
		return NULL;
	}
	inode->i_op = &pipe_inode_operations;
	atomic_set(&inode->i_count, 2);	/* sum of readers/writers */
	PIPE_WAIT(*inode) = NULL;
	PIPE_START(*inode) = PIPE_LEN(*inode) = 0;
	PIPE_RD_OPENERS(*inode) = PIPE_WR_OPENERS(*inode) = 0;
	PIPE_READERS(*inode) = PIPE_WRITERS(*inode) = 1;
	PIPE_LOCK(*inode) = 0;
	inode->i_pipe = 1;
	inode->i_mode |= S_IFIFO | S_IRUSR | S_IWUSR;
	inode->i_uid = current->fsuid;
	inode->i_gid = current->fsgid;
	inode->i_atime = inode->i_mtime = inode->i_ctime = FS_CURRENT_TIME;
	inode->i_blksize = PAGE_SIZE;
	return inode;
#else
	return NULL;
#endif
}

struct inode * __iget(struct super_block * sb, int nr, int crossmntp)
{
	static DECLARE_WAIT_QUEUE_HEAD(update_wait);
	struct inode_hash_entry * h;
	struct inode * inode;
	struct inode * empty = NULL;


	if (!sb)
		panic("VFS: iget with sb==NULL");
	h = hash(sb->s_dev, nr);
repeat:
	for (inode = h->inode; inode ; inode = inode->i_hash_next)
		if (inode->i_dev == sb->s_dev && inode->i_ino == nr)
			goto found_it;
	if (!empty) {
		h->updating++;
		empty = get_empty_inode();
		if (!--h->updating)
			wake_up(&update_wait);
		if (empty)
			goto repeat;
		return (NULL);
	}
	inode = empty;

	inode->i_sb = sb;
	inode->i_dev = sb->s_dev;
	inode->i_ino = nr;
	inode->i_flags = sb->s_flags;
	init_waitqueue_head(&inode->i_wait);
	put_last_free(inode);
	insert_inode_hash(inode);
	read_inode(inode);
	goto return_it;

found_it:
	if (!atomic_read(&inode->i_count))
		nr_free_inodes--;
	atomic_inc(&inode->i_count);
	wait_on_inode(inode);
	if (inode->i_dev != sb->s_dev || inode->i_ino != nr) {
		printk("Whee.. inode changed from under us. Tell Linus\n");
		iput(inode);
		goto repeat;
	}
	if (crossmntp && inode->i_mount) {
		struct inode * tmp = inode->i_mount;
		atomic_inc(&tmp->i_count);
		iput(inode);
		inode = tmp;
		wait_on_inode(inode);
	}
	if (empty)
		iput(empty);

return_it:
	while (h->updating)
		sleep_on(&update_wait);
	return inode;
}

/*
 * The "new" scheduling primitives (new as of 0.97 or so) allow this to
 * be done without disabling interrupts (other than in the actual queue
 * updating things: only a couple of 386 instructions). This should be
 * much better for interrupt latency.
 */
static void __wait_on_inode(struct inode * inode)
{
	DECLARE_WAITQUEUE(wait, current);

	add_wait_queue(&inode->i_wait, &wait);
repeat:
	current->state = TASK_UNINTERRUPTIBLE;
	mb();
	if (inode->i_lock) {
		TaskSwitch();
		goto repeat;
	}
	remove_wait_queue(&inode->i_wait, &wait);
	current->state = TASK_RUNNING;
}


