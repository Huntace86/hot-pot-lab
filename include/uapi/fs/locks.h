#ifndef _LINUX_LOCKS_H
#define _LINUX_LOCKS_H

#include <dim-sum/sched.h>
/*
 * Buffer cache locking - note that interrupts may only unlock, not
 * lock buffers.
 */
extern void __wait_on_buffer(struct buffer_head *);

static inline void wait_on_buffer(struct buffer_head * bh)
{
	if (bh->b_lock)
		__wait_on_buffer(bh);
}

static inline void lock_buffer(struct buffer_head * bh)
{
	if (bh->b_lock)
		__wait_on_buffer(bh);
	bh->b_lock = 1;
}

extern asmlinkage int printk_xby(const char *fmt, ...);
static inline void unlock_buffer(struct buffer_head * bh)
{
	bh->b_lock = 0;
	wake_up_all(&bh->b_wait);
}

/*
 * super-block locking. Again, interrupts may only unlock
 * a super-block (although even this isn't done right now.
 * nfs may need it).
 */
extern void __wait_on_super(struct super_block *);

static inline void wait_on_super(struct super_block * sb)
{
	if (sb->s_lock)
		__wait_on_super(sb);
}

static inline void lock_super(struct super_block * sb)
{
	if (sb->s_lock)
		__wait_on_super(sb);
	sb->s_lock = 1;
}

static inline void unlock_super(struct super_block * sb)
{
	sb->s_lock = 0;
	wake_up(&sb->s_wait);
}

#endif /* _LINUX_LOCKS_H */

