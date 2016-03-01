
/**
 * boot�ڴ������
 */
#include <base/string.h>
#include <dim-sum/base.h>
#include <dim-sum/boot_allotter.h>
#include <linux/log2.h>

#include <asm/page.h>
/**
 * boot�ڴ������ʼ�ͽ�����ַ
 */
static unsigned long BootMemoryStart = 0, BootMemoryEnd = 0;

/**
 * boot�����ڴ�������ʱ�ڴ���������ַ
 */
static unsigned long BootMemoryPtrLow, BootMemoryPtrHigh;

/**
 * boot�����ڴ����ṹ������
 */
static struct BootReserveMemBlkHeader *BootReserveMemBlkList = NULL;

/**
 * ��ʼ��Boot�����ڴ����
 */
static void InitBootReserveMemory(int ModuleID, int Size, int Align)
{
	unsigned long BootReserveMemAddr   = 0;
	unsigned int  BootReserveMemBlkNum = 0;
	unsigned int  i                    = 0;
	struct BootReserveMemBlkHeader *p  = NULL;

	/* ����Boot�����ڴ� */
	BootReserveMemAddr = (unsigned long)AllocPermanentBootMemory(ModuleID, Size, Align);
	if (BootReserveMemAddr == 0)
	{
		return;
	}

	/* ����Boot�����ڴ����Ŀ */
	BootReserveMemBlkNum = (unsigned int)(Size / BOOT_RESERVE_MEM_BLK_SIZE);
	if (BootReserveMemBlkNum == 0)
	{
		return;
	}

	/* ����Boot�����ڴ����������ڴ� */
	BootReserveMemBlkList = (struct BootReserveMemBlkHeader *)AllocPermanentBootMemory(0, 
								BootReserveMemBlkNum * sizeof(struct BootReserveMemBlkHeader), 
								sizeof(struct BootReserveMemBlkHeader));
	if (BootReserveMemBlkList == NULL)
	{
		return;
	}

	/* ��¼Boot�����ڴ����Ϣ */
	p = BootReserveMemBlkList;
	for (i = 0 ; i < BootReserveMemBlkNum; i++)
	{
		if (i == (BootReserveMemBlkNum - 1))
		{
			p->next = NULL;
		}
		else
		{
			p->next = (struct BootReserveMemBlkHeader *)((unsigned long)p + sizeof(struct BootReserveMemBlkHeader));
		}
		p->ReserveMemBlkAddr = BootReserveMemAddr + (BOOT_RESERVE_MEM_BLK_SIZE * i);
		p->ReserveMemBlkSize = BOOT_RESERVE_MEM_BLK_SIZE;
		p->ReserveMemBlkUsed = 0;
		p = p->next;
	}
	
	return;
}

/**
 * ��ʼ��Boot�ڴ������
 */
void InitBootMemory(unsigned long start, unsigned long end)
{
	memset((void *)start, 0, end - start);
	BootMemoryStart		= start;
	BootMemoryEnd		= end;

	BootMemoryPtrLow	= start;
	BootMemoryPtrHigh	= end;

	
	/* ��ʼ�������ڴ� */
	InitBootReserveMemory(0, BOOT_RESERVE_MEM_SIZE, BOOT_RESERVE_MEM_BLK_SIZE);
}

/**
 * ��boot�ڴ����з��䱣���ڴ�
 */
void* AllocStack(void)
{
	struct BootReserveMemBlkHeader *p  = NULL;

	p = BootReserveMemBlkList;
	while (p != NULL)
	{
		if (p->ReserveMemBlkUsed == 1)
		{
			p = p->next;
			continue;
		}
		else
		{
			p->ReserveMemBlkUsed = 1;
			return (void *)p->ReserveMemBlkAddr;
		}
	}

	return NULL;
}

/**
 * ��boot�ڴ������ͷű����ڴ�
 */
void FreeStack(void* Address)
{
	struct BootReserveMemBlkHeader *p  = NULL;

	p = BootReserveMemBlkList;
	while (p != NULL)
	{
		if (p->ReserveMemBlkAddr != (unsigned long)Address)
		{
			p = p->next;
			continue;
		}
		else
		{
			p->ReserveMemBlkUsed = 0;
			return;
		}
	}

	return;
}

/**
 * ��boot�ڴ����з�����ʱ���ڴ�
 * ϵͳ��ʼ����ɺ󼴷������ں�
 */
void* AllocTempBootMemory(int ModuleID, int Size, int Align)
{
	unsigned long start;
	struct BootMemoryBlockHeader *BootMemBlkHdr = NULL;

	/* boot�ڴ�δ��ʼ�� */
	if ((BootMemoryStart == 0)
		&& (BootMemoryEnd == 0))
	{
		BUG();
	}

	/**
	 * 64λϵͳĬ��8�ֽڶ���,32λϵͳĬ��4�ֽڶ���
	 */
	if (Align == 0)
	{
#ifdef CONFIG_64BIT
		Align = 8;
#else
		Align = 4;
#endif	
	}

	/* Ԥ��boot�ڴ������ռ�ڴ�ռ� */
	start = BootMemoryPtrHigh - Size;

	/* �����뷽ʽ������boot�ڴ����ʵ����ʼ��ַ */
	start = start - start % Align;

	/* �ƶ�boot��ʱ�ڴ�����ַָ�� */
	BootMemoryPtrHigh = start - sizeof(struct BootMemoryBlockHeader);
	if (BootMemoryPtrHigh < BootMemoryPtrLow)
	{
		BUG();
	}	

	/* ��¼boot�ڴ�������Ϣ */
	BootMemBlkHdr			= (struct BootMemoryBlockHeader *)(BootMemoryPtrHigh);
	BootMemBlkHdr->ModuleID = ModuleID;
	BootMemBlkHdr->Size 	= Size;
	BootMemBlkHdr->MemStart = start;

	return (void *)start;
}

/**
 * ��boot�ڴ����з������õ��ڴ�
 * ϵͳ��ʼ����ɺ󲻻᷵�ظ��ں�
 */
void* AllocPermanentBootMemory(int ModuleID, int Size, int Align)
{
	unsigned long start;
	struct BootMemoryBlockHeader *BootMemBlkHdr = NULL;

	/* boot�ڴ�δ��ʼ�� */
	if ((BootMemoryStart == 0)
		&& (BootMemoryEnd == 0))
	{
		BUG();
	}

	/**
	 * 64λϵͳĬ��8�ֽڶ���,32λϵͳĬ��4�ֽڶ���
	 */
	if (Align == 0)
	{
#ifdef CONFIG_64BIT
		Align = 8;
#else
		Align = 4;
#endif	
	}

	/* Ԥ��boot�ڴ��ͷ��ռ�ڴ�ռ� */
	start = BootMemoryPtrLow + sizeof(struct BootMemoryBlockHeader);

	/* �����뷽ʽ������boot�ڴ����ʵ����ʼ��ַ */
	start = start + Align;
	start = start - start % Align;

	/* �ƶ�boot�����ڴ�����ַָ�� */
	BootMemoryPtrLow = start + Size;
	if (BootMemoryPtrLow > BootMemoryPtrHigh)
	{
		BUG();
	}	

	/* ��¼boot�ڴ�������Ϣ */
	BootMemBlkHdr			= (struct BootMemoryBlockHeader *)(start - sizeof(struct BootMemoryBlockHeader));
	BootMemBlkHdr->ModuleID = ModuleID;
	BootMemBlkHdr->Size		= Size;
	BootMemBlkHdr->MemStart	= start;

	return (void *)start;
}

/**
 * ����boot�ڴ�
 */
void* AllocBootMemory(int ModuleID, int Size, int Align, int MemType)
{
	if (MemType == BOOT_PERMANENT_MEM)
	{
		/* ����boot�����ڴ� */
		return AllocPermanentBootMemory(ModuleID, Size, Align);
	}
	else if (MemType == BOOT_TEMP_MEM)
	{
		/* ����boot��ʱ�ڴ� */
		return AllocTempBootMemory(ModuleID, Size, Align);
	}
	else if (MemType == BOOT_RESERVE_MEM)
	{
		/* ����boot�����ڴ� */
		return AllocStack();
	}
	else
	{
		/* boot�ڴ�������ʹ��� */
		//BUG();
	}

	return NULL;
}

/**
 * �ͷ�Boot�ڴ�
 */
void FreeBootMemory(void* Address, int MemType)
{
	/**
	 * boot�ڴ�����Ϊ�����ڴ�������ʱ�ڴ����
	 * �����ڴ������ϵͳ�������ͷ�
	 * ��ʱ�ڴ������ϵͳ������ͳһ�ͷ�
	 */
	if (MemType == BOOT_PERMANENT_MEM)
	{
		/* �ͷ�boot�����ڴ� */
		return;
	}
	else if (MemType == BOOT_TEMP_MEM)
	{
		/* �ͷ�boot��ʱ�ڴ� */
		return;
	}
	else if (MemType == BOOT_RESERVE_MEM)
	{
		/* �ͷ�boot�����ڴ� */
		FreeStack(Address);
		return;
	}
	else
	{
		/* boot�ڴ��ͷ����ʹ��� */
		//BUG();
	}

	return;
}

#if 0
/*
 * allocate a large system hash table from bootmem
 * - it is assumed that the hash table must contain an exact power-of-2
 *   quantity of entries
 * - limit is the number of hash buckets, not the total allocation size
 */
void *__init alloc_large_system_hash(const char *tablename,
				     unsigned long bucketsize,
				     unsigned long numentries,
				     int scale,
				     int flags,
				     unsigned int *_hash_shift,
				     unsigned int *_hash_mask,
				     unsigned long limit)
{
	unsigned long log2qty, size;
	void *table = NULL;

	/* ����ָ���˲��� */
	if (!numentries) {
		return table;
	}
	/* rounded up to nearest power of 2 in size */
	numentries = 1UL << (long_log2(numentries) + 1);

	if ((limit != 0)
		&& (numentries > limit))
		numentries = limit;

	log2qty = long_log2(numentries);

	do {
		size = bucketsize << log2qty;
		table = AllocPermanentBootMemory(BOOT_MEMORY_MODULE_UNKNOWN, size, 0);
	} while (!table && size > PAGE_SIZE && --log2qty);

	if (!table)
		panic("Failed to allocate %s hash table\n", tablename);

	printk("%s hash table entries: %d (order: %d, %lu bytes)\n",
	       tablename,
	       (1U << log2qty),
	       long_log2(size) - PAGE_SHIFT,
	       size);

	if (_hash_shift)
		*_hash_shift = log2qty;
	if (_hash_mask)
		*_hash_mask = (1 << log2qty) - 1;

	return table;
}
#endif

