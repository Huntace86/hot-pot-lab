

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include <dim-sum/sched.h>

/**********************************************************
 *                         Macro                          *
 **********************************************************/

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/

/**********************************************************
 *                    Global Variables                    *
 **********************************************************/

/**********************************************************
 *                    Static Variables                    *
 **********************************************************/

/**********************************************************
 *                       Implements                       *
 **********************************************************/
/**
 * ��������Ż�ȡ����ṹ��
 */
asmlinkage struct task_struct *sh_ker_get_task(int task_id)
{
	struct task_struct *p_task = NULL;

	for_each_task(p_task)
	{
		if (p_task->pid == 0)
		{
			p_task = NULL;
			break;
		}
		
		if (p_task->pid == task_id)
		{
			break;
		}
	}

	return p_task;
}

/**
 * �Ƿ�����Ч�����
 */
asmlinkage int sh_ker_is_valid_task(int task_id)
{
	return (sh_ker_get_task(task_id)) ? 1 : 0;
}

/**
 * ��ȡ��ʼ�����
 */
asmlinkage int sh_ker_get_init_task_id(void)
{
	struct task_struct *p_task = &init_task;

	p_task = next_task(p_task);

	return p_task->pid;
}

/**
 * ��ȡ��һ�������
 */
asmlinkage int sh_ker_get_next_task_id(int prev_task_id)
{
	struct task_struct *p_prev_task = NULL;
	struct task_struct *p_next_task = NULL;

	p_prev_task = sh_ker_get_task(prev_task_id);

	if (p_prev_task)
	{
		p_next_task = next_task(p_prev_task);
		return p_next_task->pid;
	}

	return -1;
}

/**
 * ��ȡ������
 */
asmlinkage char *sh_ker_get_task_name(int task_id)
{
	struct task_struct *p_task = NULL;

	p_task = sh_ker_get_task(task_id);
	if (p_task)
	{
		return p_task->name;
	}

	return NULL;
}

/**
 * ��ȡ������ڵ�ַ
 */
asmlinkage unsigned long sh_ker_get_task_entry(int task_id)
{
	struct task_struct *p_task = NULL;

	p_task = sh_ker_get_task(task_id);
	if (p_task)
	{
		return (unsigned long)p_task->func;
	}

	return 0;
}

/**
 * ��ȡ�������ȼ�
 */
asmlinkage int sh_ker_get_task_prio(int task_id)
{
	struct task_struct *p_task = NULL;

	p_task = sh_ker_get_task(task_id);
	if (p_task)
	{
		return p_task->prio;
	}

	return 0;
}

/**
 * ��ȡ�������ȼ�
 */
asmlinkage int sh_ker_get_task_state(int task_id)
{
	struct task_struct *p_task = NULL;

	p_task = sh_ker_get_task(task_id);
	if (p_task)
	{
		return p_task->state;
	}

	return 0;
}

/**
 * ��ͣ��������
 */
asmlinkage int sh_ker_suspend_task(int task_id)
{
	struct task_struct *p_task = NULL;
	p_task = sh_ker_get_task(task_id);

	return ker_suspend_task(p_task);
}

/**
 * ��ͣ��������
 */
asmlinkage int sh_ker_resume_task(int task_id)
{
	struct task_struct *p_task = NULL;
	p_task = sh_ker_get_task(task_id);

	return ker_resume_task(p_task);
}

/**
 * ��ӡ����ջ��
 */
asmlinkage void sh_ker_dump_task(int task_id)
{
	struct task_struct *p_task = NULL;
	p_task = sh_ker_get_task(task_id);
	show_stack(p_task, NULL);
	return;
}

