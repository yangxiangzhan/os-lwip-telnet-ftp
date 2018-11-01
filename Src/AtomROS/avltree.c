/**
******************************************************************************
* @file           avltree.c
* @author         古么宁
* @brief          avltree file. 平衡二叉树的实现
******************************************************************************
*
* COPYRIGHT(c) 2018 GoodMorning
*
******************************************************************************
*/
/* Includes ---------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>

#include "avltree.h"


//#define AVL_DEBUG

#ifdef AVL_DEBUG
	#define avl_debug(...) printf(__VA_ARGS__)
#else
	#define avl_debug(...)
#endif





static void __avl_rotate_right(struct avl_node *node, struct avl_root *root);
static void __avl_rotate_left(struct avl_node *node, struct avl_root *root);

static int __avl_balance_right(struct avl_node *node, struct avl_root *root);
static int __avl_balance_left(struct avl_node *node, struct avl_root *root);

static int __left_hand_insert_track_back(struct avl_node *node, struct avl_root *root);
static int __right_hand_insert_track_back(struct avl_node *node, struct avl_root *root);


#define   __left_hand_delete_track_back(node,root)   __right_hand_insert_track_back(node,root) 
#define   __right_hand_delete_track_back(node,root)   __left_hand_insert_track_back(node,root) 



/**
	* @brief    __avl_rotate_right
	*           对 node 节点进行单右旋处理
	* @param    node        二叉树节点
	* @param    root        二叉树根
	* @return   
*/
static void __avl_rotate_right(struct avl_node *node, struct avl_root *root)
{
    //对以*node为根的二叉树作右旋转处理，处理之后node指向新的树根结点
    //即旋转处理之前的左子树的根结点
    struct avl_node *left = node->avl_left;
    struct avl_node *parent = avl_parent(node);

    if ((node->avl_left = left->avl_right))
        avl_set_parent(left->avl_right, node);
	
    left->avl_right = node;

    avl_set_parent(left, parent);

    if (parent)
    {
        if (node == parent->avl_right)
            parent->avl_right = left;
        else
            parent->avl_left = left;
    }
    else
        root->avl_node = left;
	
    avl_set_parent(node, left);
}


/**
* @brief    __avl_rotate_left
*           对 node 节点进行单左旋处理
* @param    node        二叉树节点
* @param    root        二叉树根
* @return
*/
static void __avl_rotate_left(struct avl_node *node, struct avl_root *root)
{
    struct avl_node *right = node->avl_right;
    struct avl_node *parent = avl_parent(node);

    if ((node->avl_right = right->avl_left))
	{
        avl_set_parent(right->avl_left, node);
	}
	
    right->avl_left = node;

    avl_set_parent(right, parent);

    if (parent)
    {
        if (node == parent->avl_left)
            parent->avl_left = right;
        else
            parent->avl_right = right;
    }
    else
	{
        root->avl_node = right;
	}
    avl_set_parent(node, right);
}


/**
* @brief    __avl_balance_left
*           当 node 节点左子树高于右子树， 对 node 节点进行左平衡处理并更新平衡因子
* @param    node        二叉树节点
* @param    root        二叉树根
* @return   对于原 node 所在位置，经过平衡处理使树的高度降低了返回 -1，否则返回0
*/
static int __avl_balance_left(struct avl_node *node, struct avl_root *root)
{
	int retl = 0;
	int left_right_child_scale;
    struct avl_node * left_child = node->avl_left;
    struct avl_node * left_left_child ;
    struct avl_node * left_right_child;
	
    if(left_child)
	{
		left_left_child = left_child->avl_left;
		left_right_child = left_child->avl_right;
		
		
		switch(avl_scale(left_child))
		{
			case AVL_BALANCED://只有在删除的时候会出现这种情况，情况非常复杂，需小心处理		
				left_right_child_scale = avl_scale(left_right_child);
				__avl_rotate_left(node->avl_left, root); //对*node的左子树作左平衡处理
				__avl_rotate_right(node, root);          //对*node作右平衡处理
				avl_set_tilt_left(left_child);
				avl_set_tilt_left(left_right_child);
				
				if (left_right_child_scale == AVL_BALANCED)
					avl_set_balanced(node);
				else 
				if (left_right_child_scale == AVL_TILT_LEFT)
					avl_set_tilt_right(node);
				else
				{
					int left_left_child_scale = avl_scale(left_left_child) ;
					avl_set_balanced(node);
					retl = __avl_balance_left(left_child, root);//这种情况需递归,并需要根据递归结果更新平衡因子
					if ((retl < 0)||(left_left_child_scale != AVL_BALANCED))
						avl_set_balanced(left_right_child);
				}
				avl_debug("L_R\r\n");
				break;

			case AVL_TILT_LEFT:
				__avl_rotate_right(node,root);
				avl_set_balanced(node);
				avl_set_balanced(left_child);
				retl = -1;//高度变低了
				avl_debug("R\r\n");
				break;
				
			case AVL_TILT_RIGHT:
				__avl_rotate_left(node->avl_left,root);  //对*node的左子树作左平衡处理
				__avl_rotate_right(node,root);      //对*node作右平衡处理
				
				switch(avl_scale(left_right_child))
				{
					case AVL_BALANCED:
						avl_set_balanced(node);
						avl_set_balanced(left_child);
						break;
					case AVL_TILT_RIGHT:
						avl_set_balanced(node);
						avl_set_tilt_left(left_child);
						break;
					case AVL_TILT_LEFT:
						avl_set_balanced(left_child);
						avl_set_tilt_right(node);
						break;
				}
				
				avl_set_balanced(left_right_child);
				retl = -1;//高度变低
				
				avl_debug("L_R\r\n");
				break;
		}
	}
	
	return retl;
}



/**
* @brief    __avl_balance_right
*           当 node 节点右子树高于左子树， 对 node 节点进行左平衡处理并更新平衡因子
* @param    node        二叉树节点
* @param    root        二叉树根
* @return   对于原 node 所在树的位置，经过平衡处理使树的高度降低了返回 -1，否则返回0
*/
static int __avl_balance_right(struct avl_node *node, struct avl_root *root)
{
	int ret = 0;
	int right_left_child_scale;
    struct avl_node * right_child = node->avl_right;
    struct avl_node * right_left_child = right_child->avl_left;
	struct avl_node * right_right_child = right_child->avl_right;
	
    if(right_child)
	{
		switch(avl_scale(right_child))
		{
			case AVL_BALANCED://删除的时候会出现这种情况，需要特别注意
				right_left_child_scale = avl_scale(right_left_child);
				
				__avl_rotate_right(node->avl_right,root);
				__avl_rotate_left(node,root);
				avl_set_tilt_right(right_left_child);
				avl_set_tilt_right(right_child);
				
				if (right_left_child_scale == AVL_BALANCED)
					avl_set_balanced(node);
				else 
				if (right_left_child_scale == AVL_TILT_RIGHT)
					avl_set_tilt_left(node);
				else
				{
					int right_right_child_scale = avl_scale(right_right_child);
					avl_set_balanced(node);
					ret = __avl_balance_right(right_child, root);//需递归一次
					if ((ret < 0)||(right_right_child_scale != AVL_BALANCED)) 
						avl_set_balanced(right_left_child);
				}	
				avl_debug("R_L\r\n");
				break;

			case AVL_TILT_RIGHT:
				avl_debug("L\r\n");
				__avl_rotate_left(node, root);
				avl_set_balanced(node);
				avl_set_balanced(right_child);
				ret = -1;//高度变低了//
				break;

			case AVL_TILT_LEFT:
				__avl_rotate_right(node->avl_right,root);
				__avl_rotate_left(node,root);
	
				avl_debug("R_L\r\n");
				switch(avl_scale(right_left_child)) //旋转后要更新平衡因子
				{
					case AVL_TILT_LEFT:
						avl_set_tilt_right(right_child);
						avl_set_balanced(node);
						break;
					case AVL_BALANCED:
						avl_set_balanced(node);
						avl_set_balanced(right_child);
						break;
					case AVL_TILT_RIGHT:
						avl_set_balanced(right_child);
						avl_set_tilt_left(node);
						break;
				}
				
				avl_set_balanced(right_left_child);
				ret = -1;//
				break;
		}
	}
	
	return ret;
}



/**
* @brief    __left_hand_insert_track_back
*           在 node 节点的左子树进行了插入，更新平衡因子，如失衡则作平衡处理
* @param    node        二叉树节点
* @param    root        二叉树根
* @return   对于原 node 所在树的位置，经过平衡处理使树的高度降低了返回 -1，否则返回0
*/
static int __left_hand_insert_track_back(struct avl_node *node, struct avl_root *root)
{
	switch(avl_scale(node))
	{
		case AVL_BALANCED  :
			avl_set_tilt_left(node);
			return 0;
			
		case AVL_TILT_RIGHT:
			avl_set_balanced(node);
			return -1;
			
		case AVL_TILT_LEFT :
			return __avl_balance_left(node,root);
	}
	
	return 0;
}



/**
* @brief    __left_hand_insert_track_back
*           在 node 节点的右子树进行了插入，更新平衡因子，如失衡则作平衡处理
* @param    node        二叉树节点
* @param    root        二叉树根
* @return   对于原 node 所在树的位置，插入并平衡后高度降低了返回 -1，否则返回0
*/
static int __right_hand_insert_track_back(struct avl_node *node, struct avl_root *root)
{
	switch(avl_scale(node))
	{
		case AVL_BALANCED:
			avl_set_tilt_right(node);//父节点右倾
			return 0;                //以 node 为根的树高度被改变，但未失衡
			
		case AVL_TILT_LEFT:
			avl_set_balanced(node);//父节点平衡
			return -1;             //以 node 为根的树高度不改变
			
		case AVL_TILT_RIGHT://以 node 为根的树已失衡，作平衡处理
			return __avl_balance_right(node,root);//
	}
	
	return 0;
}




void avl_insert(
	struct avl_root *root,
	struct avl_node * insertnode,
	struct avl_node * parent,
	struct avl_node ** avl_link)
{
	int taller = 1;

	struct avl_node *gparent = NULL;

	uint8_t parent_gparent_path = 0;
	uint8_t backtrack_path = 0;

	insertnode->avl_parent = (unsigned long)parent;
	insertnode->avl_left = insertnode->avl_right = NULL;

	*avl_link = insertnode;

	if (root->avl_node == insertnode) return;

	if (AVL_BALANCED != avl_scale(parent))
	{
		avl_set_balanced(parent);//父节点平衡
		return;//树没长高返回
	}

	backtrack_path = (insertnode == parent->avl_left) ? AVL_TILT_LEFT : AVL_TILT_RIGHT;

	//树长高了，需要回溯平衡
	while (taller && parent)
	{
		//回溯平衡过程会改变树的结构，先记录祖父节点和对应的回溯路径方向
		if ( (gparent = avl_parent(parent)) )//先赋值再判断
			parent_gparent_path = (parent == gparent->avl_right) ? AVL_TILT_RIGHT : AVL_TILT_LEFT;

		if (backtrack_path == AVL_TILT_RIGHT)
			taller += __right_hand_insert_track_back(parent, root);
		else
			taller += __left_hand_insert_track_back(parent, root);

		backtrack_path = parent_gparent_path; //回溯
		parent = gparent;
	}
}

#if 1
void avl_delete(struct avl_root *root, struct avl_node * node)
{
	struct avl_node *child, *parent;
	struct avl_node *gparent = NULL;
	int lower = 1;

	uint8_t parent_gparent_path = 0;
	uint8_t backtrack_path = 0;

	if (!node->avl_left)//如果被删节点不存在左子树
	{
		child = node->avl_right; //把右子树接入父节点中

		parent = avl_parent(node);

		if (child) avl_set_parent(child, parent);

		if (parent)
		{
			if (parent->avl_left == node)
			{
				parent->avl_left = child;
				backtrack_path = AVL_TILT_LEFT;
			}
			else
			{
				parent->avl_right = child;
				backtrack_path = AVL_TILT_RIGHT;
			}
		}
		else
		{
			root->avl_node = child;
			if (child) avl_set_balanced(child);
			return;
		}
	}
	else if (!node->avl_right) //如果被删节点存在左子树，不存在右子树
	{
		child = node->avl_left;

		parent = avl_parent(node);

		avl_set_parent(child, parent);

		if (parent)
		{
			if (parent->avl_left == node)
			{
				parent->avl_left = child;
				backtrack_path = AVL_TILT_LEFT;
			}
			else
			{
				parent->avl_right = child;
				backtrack_path = AVL_TILT_RIGHT;
			}
		}
		else
		{
			root->avl_node = child;
			if (child) avl_set_balanced(child);
			return;
		}
	}
	else //被删节点即存在左子树，也存在右子树
	{
		struct avl_node *old = node, *left;

		node = node->avl_right;

		while ((left = node->avl_left) != NULL) node = left;//找到右子树下最小的替代点

		if (avl_parent(old)) //旧点所在父节点
		{
			if (avl_parent(old)->avl_left == old)
				avl_parent(old)->avl_left = node;
			else
				avl_parent(old)->avl_right = node;

		}
		else root->avl_node = node;

		child = node->avl_right;  //最小的替代点的右节点
		parent = avl_parent(node);//最小的替代点的父节点

		if (parent == old) //要删除的节点的右子树没有左子树，替代点(1)没有右节点,(2)存在一个右节点  
		{
			backtrack_path = AVL_TILT_RIGHT;

			parent = node;
			node->avl_parent = old->avl_parent;
			node->avl_left = old->avl_left;
			avl_set_parent(old->avl_left, node);
		}
		else  //要删除的节点的右子树有左子树
		{
			backtrack_path = AVL_TILT_LEFT;

			parent->avl_left = child;          //最小的替代点的右节点的父节点
			node->avl_right = old->avl_right;
			avl_set_parent(old->avl_right, node);

			node->avl_parent = old->avl_parent;
			node->avl_left = old->avl_left;
			avl_set_parent(old->avl_left, node);

			if (child) avl_set_parent(child, parent); //要删除的节点的右子树的左子树末尾点存在右节点
		}
	}

	//树低了，需要回溯平衡，直到回溯到根节点
	while (lower && parent)
	{
		if ( (gparent = avl_parent(parent)) )//(parent && (gparent = avl_parent(parent)))//先赋值再判断
			parent_gparent_path = (parent == gparent->avl_right) ? AVL_TILT_RIGHT : AVL_TILT_LEFT;

		if (backtrack_path == AVL_TILT_RIGHT) //经过回溯调整会改变树的结构，所以先记录 gparent 和回溯路径 
			lower = __right_hand_delete_track_back(parent, root);
		else
			lower = __left_hand_delete_track_back(parent, root);

		backtrack_path = parent_gparent_path;
		parent = gparent;
	}
}

#endif

/*
 * This function returns the first node (in sort oright_left_childer) of the tree.
 */
struct avl_node *avl_first(const struct avl_root *root)
{
    struct avl_node    *n;

    n = root->avl_node;
    if (!n)
        return NULL;
    while (n->avl_left)
        n = n->avl_left;
    return n;
}

struct avl_node *avl_last(const struct avl_root *root)
{
    struct avl_node    *n;

    n = root->avl_node;
    if (!n)
        return NULL;
    while (n->avl_right)
        n = n->avl_right;
    return n;
}

struct avl_node *avl_next(const struct avl_node *node)
{
    struct avl_node *parent;

    if (avl_parent(node) == node)
        return NULL;

    /* If we have a right-hand child, go down and then left as far
       as we can. */
    if (node->avl_right) {
        node = node->avl_right; 
        while (node->avl_left)
            node=node->avl_left;
        return (struct avl_node *)node;
    }

    /* No right-hand children.  Everything down and left is
       smaller than us, so any 'next' node must be in the general
       direction of our parent. Go up the tree; any time the
       ancestor is a right-hand child of its parent, keep going
       up. First time it's a left-hand child of its parent, said
       parent is our 'next' node. */
    while ((parent = avl_parent(node)) && node == parent->avl_right)
        node = parent;

    return parent;
}

struct avl_node *avl_prev(const struct avl_node *node)
{
    struct avl_node *parent;

    if (avl_parent(node) == node)
        return NULL;

    /* If we have a left-hand child, go down and then right as far
       as we can. */
    if (node->avl_left) {
        node = node->avl_left; 
        while (node->avl_right)
            node=node->avl_right;
        return (struct avl_node *)node;
    }

    /* No left-hand children. Go up till we find an ancestor which
       is a right-hand child of its parent */
    while ((parent = avl_parent(node)) && node == parent->avl_left)
        node = parent;

    return parent;
}

void avl_replace_node(struct avl_node *victim, struct avl_node *new,
             struct avl_root *root)
{
    struct avl_node *parent = avl_parent(victim);

    /* Set the surrounding nodes to point to the replacement */
    if (parent) {
        if (victim == parent->avl_left)
            parent->avl_left = new;
        else
            parent->avl_right = new;
    } else {
        root->avl_node = new;
    }
    if (victim->avl_left)
        avl_set_parent(victim->avl_left, new);
    if (victim->avl_right)
        avl_set_parent(victim->avl_right, new);

    /* Copy the pointers/colour from the victim to the replacement */
    *new = *victim;
}
