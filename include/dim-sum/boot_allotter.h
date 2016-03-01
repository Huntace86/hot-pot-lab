/**
 * boot�ڴ������
 */
#ifndef _PHOENIX_BOOT_ALLOTTER_H
#define _PHOENIX_BOOT_ALLOTTER_H

#define HASH_HIGHMEM	0x00000001	/* Consider highmem? */
#define HASH_EARLY	0x00000002	/* Allocating during early boot? */

/**
 * Ҫ����boot�ڴ��ģ���ʶ���ڴ氲ȫ����
 * ��ͬ��ģ�飬�����ڲ�ͬ���ڴ������з���boot�ڴ�
 * ���ɸ�ģ�鸺�������ⶨ��ģ���ʶ
 */
// �����ֵ�ģ�飬Ĭ���ô˱�ʶ
#define	BOOT_MEMORY_MODULE_UNKNOWN	0
// IOģ��
#define	BOOT_MEMORY_MODULE_IO		1
// ����ģ��
#define	BOOT_MEMORY_MODULE_SCHED	2

/**
 * boot�ڴ�����Ϊ�����ڴ�������ʱ�ڴ����
 * �����ڴ������ϵͳ�������ͷ�
 * ��ʱ�ڴ������ϵͳ������ͳһ�ͷ�
 */
// boot�����ڴ�����ʶ
#define BOOT_PERMANENT_MEM		1
// boot��ʱ�ڴ�����ʶ
#define BOOT_TEMP_MEM		2
// boot�����ڴ�����ʶ
#define BOOT_RESERVE_MEM	3

/**
 * �����ȥ��boot�����ṹ
 */
struct BootMemoryBlockHeader
{
	int 			ModuleID;	/* boot�ڴ�����ID */
	int				Size;		/* boot�ڴ�����С*/
	unsigned long	MemStart;	/* boot�ڴ������ʼ��ַ */
};

// �����ڴ��С
#define BOOT_RESERVE_MEM_SIZE		(1024 * 1024)

// �����ڴ���С
#define BOOT_RESERVE_MEM_BLK_SIZE	(8 * 1024)

/**
 * boot�����ڴ����ṹ
 */
struct BootReserveMemBlkHeader
{
	struct BootReserveMemBlkHeader *next;	/* ָ����һ��boot�����ڴ�����ṹ�� */
	unsigned long ReserveMemBlkAddr;		/* �����ڴ����ʼ��ַ */
	unsigned int  ReserveMemBlkSize;		/* �����ڴ���С */
	unsigned int  ReserveMemBlkUsed;		/* �Ƿ�ʹ�ñ����ڴ�� */
};

void InitBootMemory(unsigned long start, unsigned long end);
void* AllocTempBootMemory(int ModuleID, int Size, int Align);
void* AllocPermanentBootMemory(int ModuleID, int Size, int Align);
void* AllocStack(void);
void  FreeStack(void * Address);

/**
 * ����boot�ڴ�
 */
void* AllocBootMemory(int ModuleID, int Size, int Align, int MemType);

/**
 * �ͷ�boot�ڴ�
 */
void FreeBootMemory(void* Address, int MemType);

#endif

