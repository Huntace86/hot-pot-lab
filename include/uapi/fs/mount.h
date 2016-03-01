#ifndef _DIM_SUM_MOUNT_H
#define _DIM_SUM_MOUNT_H

#define VFSMOUNTING 2  //in the state of mount,but not mount succses already
#define VFSMOUNTED 3	// filesysterm has already mounted
#define INDF 8	// 
#define NOTINDF 9	// 


/**
 * �ļ�ϵͳ��װ��
 */
struct vfsmount
{
	/**
	 * ����ɢ�б������ָ�롣
	 */
	struct list_head mnt_hash;
	/**
	 * ָ���ļ�ϵͳ������ļ�ϵͳ��װ�����ϡ�
	 */
	struct vfsmount *mnt_parent;	/* fs we are mounted on */
	/**
	 * ��װ��Ŀ¼�ڵ㡣
	 */
	struct dentry *mnt_mountpoint;	/* dentry of mountpoint */
	/**
	 * ָ������ļ�ϵͳ��Ŀ¼��dentry��
	 */
	struct dentry *mnt_root;	/* root of the mounted tree */
	/**
	 * ���ļ�ϵͳ�ĳ��������
	 */
	struct super_block *mnt_sb;	/* pointer to superblock */
	/**
	 * ���������ļ�ϵͳ�����������ͷ
	 */
	struct list_head mnt_mounts;	/* list of children, anchored here */
	/**
	 * �Ѱ�װ�ļ�ϵͳ����ͷ��ͨ�����ֶν�����븸�ļ�ϵͳ��mnt_mounts�����С�
	 */
	struct list_head mnt_child;	/* and going through their mnt_child */
	/**
	 * ���ü���������ֹ�ļ�ϵͳ��ж�ء�
	 */
	atomic_t mnt_count;
	/**
	 * mount��־
	 */
	int mnt_flags;
	/**
	 * ����ļ�ϵͳ���Ϊ���ڣ������������־��
	 */
	int mnt_expiry_mark;		/* true if marked for expiry */
	/**
	 * �豸�ļ�����
	 */
	char *mnt_devname;		/* Name of device e.g. /dev/dsk/hda1 */
	/**
	 * �Ѱ�װ�ļ�ϵͳ��������namespace����ָ��?
	 * ͨ�����ֶν�����뵽namespace��list�����С�
	 */
	struct list_head mnt_list;
	/**
	 * �ļ�ϵͳ��������ָ�롣
	 */
	struct list_head mnt_fslink;	/* link in fs-specific expiry list */
	/**
	 * ���������ռ�ָ��
	 */
	struct namespace *mnt_namespace; /* containing namespace */
};

static inline struct vfsmount *mntget(struct vfsmount *mnt)
{
    if (mnt) {
		atomic_inc(&mnt->mnt_count);
    }
	return mnt;
}

extern void __mntput(struct vfsmount *mnt);

static inline void mntput(struct vfsmount *mnt)
{
	if (mnt) 
	{
		if (atomic_dec_and_test(&mnt->mnt_count))
		{
			__mntput(mnt);
		}
	}
}

extern struct vfsmount *lookup_mnt(struct vfsmount *, struct dentry *);

/**
 * �����Ѿ���װ�ļ�ϵͳ������
 */
extern spinlock_t vfsmount_lock;


#endif	//_DIM_SUM_MOUNT_H
