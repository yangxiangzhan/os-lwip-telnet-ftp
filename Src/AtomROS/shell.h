
#ifndef __SHELL_H__
#define __SHELL_H__

// 以下为 shell 所依赖的基本库
//#include "rbtree.h" //命令索引用红黑树进行查找匹配
#include "avltree.h"//命令索引用avl树进行查找匹配
#include "ustdio.h"


#define     DEFAULT_INPUTSIGN       "shell>"

#define     KEYCODE_CTRL_C            0x03
#define     KEYCODE_NEWLINE           0x0A
#define     KEYCODE_ENTER             0x0D   //键盘的回车键
#define     KEYCODE_BACKSPACE         0x08   //键盘的回退键
#define     KEYCODE_ESC               0x1b
#define     KEYCODE_TAB               '\t'   //键盘的tab键



enum INPUT_PARAMETER
{
	PARAMETER_EMPTY,
	PARAMETER_CORRECT,
	PARAMETER_HELP,
	PARAMETER_ERROR
};



/*
-----------------------------------------------------------------------
	调用宏 vShell_RegisterCommand(pstr,pfunc) 注册命令
	注册一个命令号的同时会新建一个与命令对应的控制块
	在 shell 注册的函数类型统一为 void(*CmdFuncDef)(void * arg);
	arg 为控制台输入命令后所跟的参数输入
-----------------------------------------------------------------------
*/
#define   vShell_RegisterCommand(pstr,pfunc)\
	do{\
		static struct shell_cmd st##pfunc = {0};\
		_Shell_RegisterCommand(pstr,pfunc,&st##pfunc);\
	}while(0)


#define COMMANDLINE_MAX_LEN    36  //命令带上参数的字符串输入最长记录长度
#define COMMANDLINE_MAX_RECORD 4      //控制台记录条目数


//#define vShell_InitPrint(fn) do{print_CurrentOut(fn);print_DefaultOut(fn);}while(0)

#define iShell_CmdLen(pCommand)  (((pCommand)->ID >> 21) & 0x001F)



typedef void (*cmd_fn_def)(void * arg);


typedef struct shell_cmd
{
	uint32_t	  ID;	 //命令标识码
	char *		  pName; //记录每条命令字符串的内存地址
	cmd_fn_def	  Func;  //记录命令函数指针
	//struct rb_node cmd_node;//红黑树节点
	struct avl_node cmd_node;//avl树节点
}
shellcmd_t;


typedef struct shell_buf 
{
  fnFmtOutDef puts;
  char   * bufmem;
  uint32_t index;
}
shellbuf_t;

#define vShell_InitBuf(pStShellBuf,shellputs) \
	do{\
		static char bufmem[COMMANDLINE_MAX_LEN] = {0};\
		(pStShellBuf)->bufmem = bufmem;    \
		(pStShellBuf)->index  = 0;         \
		(pStShellBuf)->puts = shellputs;   \
	}while(0)


//extern char * shell_input_sign;
extern char  shell_input_sign[];
extern struct avl_root shell_avltree_root;
	
void _Shell_RegisterCommand(char * cmdname, cmd_fn_def func,struct shell_cmd * newcmd);//注册命令

void vShell_Input(struct shell_buf * pStShellbuf,char * ptr,uint8_t len);

int  iShell_ParseParam  (char * argStr,int * argc,int argv[]);

void vShell_Init(char * sign,fnFmtOutDef default_print);
	
void vShell_InputSign(char * sign);

#endif
