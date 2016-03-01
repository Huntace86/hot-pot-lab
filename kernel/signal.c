#include <asm/signal.h>
#include <linux/linkage.h>
#include <asm/siginfo.h>

int do_signal(struct pt_regs *regs)
{
	return 0;
}

asmlinkage int sys_rt_sigsuspend(const sigset_t *mask, size_t size)
{
	return -38;
}

asmlinkage long sys_rt_sigaction(int sig,
		 const struct sigaction *act,
		 struct sigaction *oact,
		 size_t sigsetsize)
{
	return -38;
}

asmlinkage long sys_rt_sigprocmask(int how, sigset_t __user *set, sigset_t __user *oset, size_t sigsetsize)
{
	return -38;
}

asmlinkage long sys_rt_sigpending(sigset_t __user *set, size_t sigsetsize)
{
	return -38;
}

/**
 * Killϵͳ���ô�����
 * pid>0 ��ʾ��sig�źŷ��͵���PID==pid�Ľ����������߳��顣
 * pid==0��ʾ���źŷ��͵�����ý���ͬ��Ľ��������߳��顣
 * pid==-1��ʾ���źŷ��͵����н��̡�����swapper��init��current
 * pid < -1���źŷ��͵�������-pid�н��̵������߳��ߡ�
 * ��Ȼkill�ܹ����ͱ����32-64֮���ʵʱ�źš�����������ȷ����һ���µ�Ԫ�ؼ��뵽Ŀ����̵Ĺ����źŶ��С�
 * ��ˣ�����ʵʱ�ź���Ҫͨ��rt_sigqueueinfoϵͳ���ý��С�
 */
asmlinkage long
sys_kill(int pid, int sig)
{
	return -38;
}

/**
 * Rt_sigqueueinfo��ϵͳ���ô�����
 */
asmlinkage long
sys_rt_sigqueueinfo(int pid, int sig, siginfo_t *uinfo)
{
	return -38;
}

asmlinkage long
sys_rt_sigtimedwait(const sigset_t __user *uthese,
		    siginfo_t __user *uinfo,
		    const struct timespec __user *uts,
		    size_t sigsetsize)
{
	return -38;
}


