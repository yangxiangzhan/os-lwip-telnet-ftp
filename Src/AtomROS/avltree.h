#ifndef __AVL_TREE_H__
#define __AVL_TREE_H__

//------------------------------------------------------------------
#ifndef offsetof
	#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)// 获取"MEMBER成员"在"结构体TYPE"中的位置偏移
#endif

#ifndef container_of
	#if 1
		// 根据"结构体(type)变量"中的"域成员变量(member)的指针(ptr)"来获取指向整个结构体变量的指针
		#define container_of(ptr, type, member)  ((type*)((char*)ptr - offsetof(type, member)))
		// 此宏定义原文为 GNU C 所写，如下，有些编译器只支持 ANSI C /C99 的，所以作以上修改
	#else
		#define container_of(ptr, type, member) ({          \
		const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
		(type *)( (char *)__mptr - offsetof(type,member) );})
	#endif
#endif
//------------------------------------------------------------------

struct avl_node
{
    unsigned long  avl_parent; 
    struct avl_node *avl_left;
    struct avl_node *avl_right;
};


struct avl_root
{
    struct avl_node *avl_node;
};


#define avl_parent(r)         ((struct avl_node *)((r)->avl_parent & (~3)))

//------------------------------------------------------------------
//获取 avl 节点的平衡因子，值如下
#define avl_scale(r)        ((r)->avl_parent & 3) 
#define AVL_BALANCED   0 //节点平衡
#define AVL_TILT_RIGHT 1 //节点右边比左边高，用 0b01 表示，右倾
#define AVL_TILT_LEFT  2 //节点左边比右边高，用 0b10 表示，左倾


//------------------------------------------------------------------
//设置 avl 节点的平衡因子
//设置 avl 节点为平衡节点
#define avl_set_balanced(r)   do {((r)->avl_parent) &= (~3);}while(0)

//设置 avl 节点为右倾节点
#define avl_set_tilt_right(r) do {(r)->avl_parent=(((r)->avl_parent & ~3)|AVL_TILT_RIGHT);} while (0)

//设置 avl 节点为左倾节点
#define avl_set_tilt_left(r)  do {(r)->avl_parent=(((r)->avl_parent & ~3)|AVL_TILT_LEFT);} while (0)

//------------------------------------------------------------------


#define  avl_entry(ptr, type, member) container_of(ptr, type, member)



// 设置 avl 节点的父节点为 p
static inline void avl_set_parent(struct avl_node *avl, struct avl_node *p)
{
    avl->avl_parent = (avl->avl_parent & 3) | (unsigned long)p;
}



void avl_insert(
               struct avl_root *root,
               struct avl_node * insertnode, 
               struct avl_node * insertparent,
			   struct avl_node ** avl_link);

void avl_delete(struct avl_root *root,struct avl_node * node);
			   
/* Find logical next and previous nodes in a tree */
extern struct avl_node *avl_next(const struct avl_node *);
extern struct avl_node *avl_prev(const struct avl_node *);
extern struct avl_node *avl_first(const struct avl_root *);
extern struct avl_node *avl_last(const struct avl_root *);
void avl_replace_node(struct avl_node *victim, struct avl_node *new,
             struct avl_root *root);
#endif
