#ifndef _unstandard_format_
#define _unstandard_format_

typedef void (*fnFmtOutDef)(char * strbuf,uint16_t len);


extern  fnFmtOutDef current_puts;
extern  fnFmtOutDef default_puts;


#define print_CurrentOut(fn) do{current_puts = fn;}while(0)
#define print_DefaultOut(fn) do{default_puts = fn;}while(0)


#define printl(ptr,len)  if (current_puts) current_puts(ptr,len)

void    printk(char* fmt, ...);

extern const  char	none        [];  
extern const  char	black       [];  
extern const  char	dark_gray   [];  
extern const  char	blue        [];  
extern const  char	light_blue  [];  
extern const  char	green       [];  
extern const  char	light_green [];  
extern const  char	cyan        [];  
extern const  char	light_cyan  [];  
extern const  char	red         [];  
extern const  char	light_red   [];  
extern const  char	purple      [];  
extern const  char	light_purple[];  
extern const  char	brown       [];  
extern const  char	yellow      [];  
extern const  char	light_gray  [];  
extern const  char	white       []; 
extern char  * default_color;

#define color_printk(color,...) \
	do{\
		printk((char *)color);\
		printk(__VA_ARGS__);  \
		printk(default_color);\
	}while(0)

#define Warnings(...) \
	do{\
		printk("%sWarning:",(char *)red);\
		printk(__VA_ARGS__);  \
		printk(default_color);\
	}while(0)
	
	
#define Errors(...)   \
	do{\
		printk("%sERROR:",(char *)light_red);\
		printk(__VA_ARGS__);  \
		printk(default_color);\
	}while(0)

		
#define Debug_Here() printk("%sHere is %s()-%d\r\n%s",(char *)green,__FUNCTION__,__LINE__,default_color)

#define Error_Here() printk("%sError on %s()-%d\r\n%s",(char *)light_red,__FUNCTION__,__LINE__,default_color)


#endif

