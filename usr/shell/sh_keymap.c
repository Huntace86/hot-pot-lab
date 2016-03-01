

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include "sh_keymap.h"
#include "sh_readline.h"
#include "sh_history.h"
#include "sh_completion.h"

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
/* �����ַ������б� */
static sh_key_map_t sh_key_map[] = 
{
	{CTRL('A'), 	ctrl_A_key_handler},
	{CTRL('B'), 	ctrl_B_key_handler},	/* ������� */
	{CTRL('C'), 	ctrl_C_key_handler},	/* �������� */
	{CTRL('D'), 	ctrl_D_key_handler},
	{CTRL('E'), 	ctrl_E_key_handler},	/* ����ƶ��������ַ���ĩβ */
	{CTRL('F'), 	ctrl_F_key_handler},	/* ������� */
	{CTRL('G'), 	NULL},
	{CTRL('H'), 	ctrl_H_key_handler},	/* backspce�� */
	{CTRL('I'), 	ctrl_I_key_handler},	/* tab�� */
	{CTRL('J'), 	NULL},
	{CTRL('K'), 	ctrl_K_key_handler},
	{CTRL('L'), 	ctrl_L_key_handler},
	{CTRL('M'), 	NULL},
	{CTRL('N'), 	ctrl_N_key_handler},	/* pagedown�� */
	{CTRL('O'), 	NULL},
	{CTRL('P'), 	ctrl_P_key_handler},	/* pageup�� */
	{CTRL('Q'), 	NULL},
	{CTRL('R'), 	NULL},
	{CTRL('S'), 	NULL},
	{CTRL('T'), 	ctrl_T_key_handler},
	{CTRL('U'), 	ctrl_U_key_handler},
	{CTRL('V'), 	NULL},
	{CTRL('W'), 	ctrl_W_key_handler},	/* etb�� */
	{CTRL('X'), 	NULL},
	{CTRL('Y'), 	NULL},
	{CTRL('Z'), 	ctrl_Z_key_handler},
	{VK_DELETE, 	delete_key_handler},
	{0, 		NULL}
};

/**********************************************************
 *                       Implements                       *
 **********************************************************/
 
int esc_key_handler(void)
{
	char c	= 0;	 /* ��ȡ�ַ� */
	int ret = 0;	 /* ����ֵ */

esc_read:

	ret = sh_read(&c, 1);
	if(ret <= 0)
	{
		/* ��ȡ�ַ����� */
		c = 0;
		goto esc_ret;
	}

	/* ������ȡ�ַ� */
	if (c == '[')
	{
		goto esc_read;
	}

	/* ��esc��ֵת��Ϊ�����ַ� */
	switch (c)
	{
		case 'A':	 /* UP */
			c = CTRL('P');	
			break;
			
		case 'B':	 /* DOWN */
			c = CTRL('N');	
			break;
			
		case 'C':	 /* RIGHT */
			c = CTRL('F');	 
			break;		 
			
		case 'D':	 /* LEFT */
			c = CTRL('B');	
			break;
			
		case '3':	 /* DELETE */
			ret = sh_read(&c, 1); 
			if(ret <= 0)
			{
				/* ��ȡ�ַ����� */
				c = 0;
				goto esc_ret;
			}

			if (c == '~')
			{
				c = VK_DELETE;
			}
			break;
			
		default:
			c = 0;
	}

esc_ret: 

	return c;	 
}

sh_key_func_t *lookup_sh_key_func(int sh_key_value)
{
	int sh_key_map_index = 0;

	while (sh_key_map[sh_key_map_index].sh_key_value != 0)
	{
		if (sh_key_map[sh_key_map_index].sh_key_value == sh_key_value)
		{
			return sh_key_map[sh_key_map_index].sh_key_func;
		}

		sh_key_map_index++;
	}

	return NULL;
}

static void input_back(int back, char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	int this_len	= *len;
	
	while (back--) 
	{
		if (this_len == this_cursor) 
		{
			input[--this_cursor] = 0;
			sh_write("\b \b", 3);
		} 
		else 
		{
			int i;
			int len;
			/* ���λ������ƶ�һλ */
			this_cursor--; 
			/* �ӹ������λ�ÿ�ʼ�������ַ�����������ƶ�һλ */
			for (i = this_cursor; i <= this_len; i++)
			{
				input[i] = input[i + 1];
			}
			/* ����ӹ������λ���ַ������� */
			len = strlen(input + this_cursor);
			/* ����˸�������ҹ��λ������ƶ�һλ��������Ҫɾ�����ַ� */
			sh_write("\b", 1);
			/* ����ӹ������λ�ÿ�ʼ���ַ��� */
			sh_write(input + this_cursor, len);
			/* ����ո񣬸������һ���ַ� */
			sh_write(" ", 1);
			/* ������ */
			for (i = 0; i <= len; i++)
			{
				sh_write("\b", 1);
			}
		}
		this_len--;
	}

	*cursor = this_cursor;
	*len	= this_len;
	
	return;
}

int ctrl_A_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_B_key_handler(char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	
	if (this_cursor) 
	{
		sh_write("\b", 1);
		this_cursor--;
	}

	*cursor = this_cursor;
	
	return RET_READ_CHAR_CONT;
}

int ctrl_C_key_handler(char *input, int *cursor, int *len)
{	
	sh_write("\a", 1);

	return RET_READ_CHAR_DONE;
}

int ctrl_D_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_E_key_handler(char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	int this_len	= *len;

	if (this_cursor < this_len) 
	{
		sh_write(&input[this_cursor], this_len - this_cursor);
		this_cursor = this_len;
	}

	*cursor = this_cursor;
	*len	= this_len;
	
	return RET_READ_CHAR_CONT;
}

int ctrl_F_key_handler(char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	int this_len	= *len;
	
	if (this_cursor < this_len) 
	{
		sh_write(&input[this_cursor], 1);
		this_cursor++;
	}

	*cursor = this_cursor;
	*len	= this_len;
	
	return RET_READ_CHAR_CONT;
}

int ctrl_H_key_handler(char *input, int *cursor, int *len)
{
	int back		= 0;
	int this_len	= *len;
	int this_cursor = *cursor;

	if (this_len == 0 || this_cursor == 0)
	{
		sh_write("\a", 1);
		return RET_READ_CHAR_CONT;
	}

	back = 1;
	 
	if (back) 
	{
		input_back(back, input, cursor, len);
		return RET_READ_CHAR_CONT;
	}

	return RET_READ_CHAR_DONE;
}

int ctrl_I_key_handler(char *input, int *cursor, int *len)
{
	sh_completion_handler(input, cursor, len);
	return RET_READ_CHAR_CONT;
}

int ctrl_K_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_L_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_N_key_handler(char *input, int *cursor, int *len)
{
	char *history_input = NULL;

	sh_clean_input(input, cursor, len);
	
	history_input = get_next_history();
	if (history_input)
	{
		
		strcpy(input, history_input);
		*cursor = *len = strlen(input);
		sh_write(input, *len);
	}

	return RET_READ_CHAR_CONT;
}

int ctrl_P_key_handler(char *input, int *cursor, int *len)
{
	char *history_input = NULL;

	sh_clean_input(input, cursor, len);
	
	history_input = get_prev_history();
	if (history_input)
	{
		
		strcpy(input, history_input);
		*cursor = *len = strlen(input);
		sh_write(input, *len);
	}
	
	return RET_READ_CHAR_CONT;
}

int ctrl_T_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_U_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_W_key_handler(char *input, int *cursor, int *len)
{
	int back		= 0;
	int nc			= *cursor;
	int this_cursor = *cursor;
	int this_len	= *len;
		
	if (this_len == 0 || this_cursor == 0)
	{
		return RET_READ_CHAR_CONT;
	}

	while (nc && input[nc - 1] == ' ') 
	{
		nc--;
		back++;
	}
	
	while (nc && input[nc - 1] != ' ') 
	{
		nc--;
		back++;
	}

	if (back) 
	{
		input_back(back, input, cursor, len);
		return RET_READ_CHAR_CONT;
	}

	return RET_READ_CHAR_DONE;
}

int ctrl_Z_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int delete_key_handler(char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	int this_len	= *len;
	
	if (this_len != this_cursor)
	{
		int i;
		int len;
		/* �ӹ������λ�ÿ�ʼ�������ַ�����������ƶ�һλ */
		for (i = this_cursor; i < this_len; i++)
			input[i] = input[i + 1];
		/* �����ַ������һ���ַ����� */
		input[this_len--] = '\0';
		/* ����ӹ������λ���ַ������� */
		len = strlen(input + this_cursor);
		/* ����ӹ������λ�ÿ�ʼ���ַ��� */
		sh_write(input + this_cursor, len);
		/* ����ո񣬸������һ���ַ� */
		sh_write(" ", 1);	
		/* ������ */
	
		for (i = 0; i <= len; i++)
			sh_write("\b", 1);
	}

	*cursor = this_cursor;
	*len	= this_len;
	
	return RET_READ_CHAR_CONT;
}

int normal_char_type_handler(char c, char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	int this_len	= *len;

	/* �س� */
	if (c == '\n')
	{
		sh_write("\n", 1);
		return RET_READ_CHAR_DONE;
	}

	/* ���� */
	if (c == '\r')
	{
		sh_write("\r\n", 2);
		return RET_READ_CHAR_DONE;
	}
	
	if (this_cursor == this_len) 
	{
		/* 
		 * �����ַ������룬���λ���������ַ�������ֵ��ȣ�
		 * ���������ַ���ĩβ�����µ��ַ�
		 */
		 
		/* append to end of line */
		if ((short)c == 20 && this_cursor == 0)
		{
			return RET_READ_CHAR_DONE;
		}
	
		input[this_cursor] = c;
		if (this_len < (RL_BUF_SIZE - 1)) 
		{
			/* δ���������ַ�����󳤶ȣ����λ�ú������ַ�������ֵ��1 */
			this_len++;
			this_cursor++;
		} 
		else 
		{
			/* ���������ַ�����󳤶ȣ�������������ַ��� */
			sh_write("\a", 1);
			return RET_READ_CHAR_DONE;
		}
	}
	else 
	{
		/* 
		 * ���λ���������ַ�������ֵ����ȣ���Ҫ���������������ַ���
		 * �м�����µ��ַ�
		 */
		 
		/* insert */
		int i = 0;
		/* move everything one character to the right */
		if (this_len >= (RL_BUF_SIZE - 2)) 
		{
			this_len--;
		}
		
		for (i = this_len; i >= this_cursor; i--)
		{
			input[i + 1] = input[i];
		}
	
		/* sh_write what we've just added */
		input[this_cursor] = c;
		
		sh_write(&input[this_cursor], this_len - this_cursor + 1);
		for (i = 0; i < (this_len - this_cursor + 1); i++)
		{
			sh_write("\b", 1);
		}
	
		this_len++;
		this_cursor++;
	}

	*cursor = this_cursor;
	*len	= this_len;

	return RET_READ_CHAR_CONT;
}

