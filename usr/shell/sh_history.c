

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include "sh_utils.h"
#include "sh_history.h"

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
static char *history[MAX_HISTORY] = {0, };	/* ��ʷ�����¼ */
static int history_count = 0;				/* ��ʷ�����¼��Ŀ */
static int cur_history	 = 0;				/* ��ǰ��ʷ�����¼ */

void add_one_history(char *cmd)
{
	int i = 0;

	if (cmd == NULL || cmd[0] == '\0')
	{
		return;
	}

	if (history_count >= MAX_HISTORY)
	{
		/* �ﵽ��ʷ�����¼��Ŀ����ʱ���ͷŵ�һ����ʷ�����¼ */
		free(history[0]);
		for (i = 0; i < MAX_HISTORY - 1; i++)
		{
			history[i] = history[i + 1];
		}
		history_count = MAX_HISTORY - 1;
	}

	history[history_count] = malloc(strlen(cmd) + 1);
	memset(history[history_count], 0x0, strlen(cmd) + 1);
	strcpy(history[history_count], cmd);
	history_count++;

	cur_history  = history_count;

	return;
}

void destroy_all_history(void)
{
	int i;
	for (i = 0; i < history_count; i++)
	{
		free(history[i]);
		history[i] = NULL;
	}

	return;
}

char *get_prev_history(void)
{
	if (cur_history == 0)
	{
		cur_history = history_count;
	}
	
	cur_history--;

	return history[cur_history];
}

char *get_next_history(void)
{
	if (cur_history < history_count - 1)
	{
		cur_history++;
	}
	else
	{
		cur_history = 0;
	}

	return history[cur_history];
}

