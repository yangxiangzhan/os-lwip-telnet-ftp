/* test.c */
#include <stdio.h>
#include <stdlib.h>
#include "rbtree.h"

struct mytype 
{
    struct rb_node my_node;
    int num;
};

struct mytype *my_search(struct rb_root *root, int num)
{
    struct rb_node *node = root->rb_node;

    while (node) 
	{
		struct mytype *data = container_of(node, struct mytype, my_node);

		if (num < data->num)
		{
		    node = node->rb_left;
		}
		else if (num > data->num)
		{
		    node = node->rb_right;
		}
  		else return data;
    }
    
    return NULL;
}

int my_insert(struct rb_root *root, struct mytype *data)
{
    struct rb_node **tmp = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*tmp) 
    {
		struct mytype *this = container_of(*tmp, struct mytype, my_node);

		parent = *tmp;
		if (data->num < this->num)
		    tmp = &((*tmp)->rb_left);
		else if (data->num > this->num)
		    tmp = &((*tmp)->rb_right);
		else 
		    return -1;
    }
    
    /* Add new node and rebalance tree. */
    rb_link_node(&data->my_node, parent, tmp);
    rb_insert_color(&data->my_node, root);
    
    return 0;
}

void my_delete(struct rb_root *root, int num)
{
    struct mytype *data = my_search(root, num);
    if (!data) { 
	fprintf(stderr, "Not found %d.\n", num);
	return;
    }
    
    rb_erase(&data->my_node, root);
    free(data);
}

void print_rbtree(struct rb_root *tree)
{
    struct rb_node *node;
    
    for (node = rb_first(tree); node; node = rb_next(node))
	printf("%d ", rb_entry(node, struct mytype, my_node)->num);
    
    printf("\n");
}

int main(int argc, char *argv[])
{
    struct rb_root mytree = RB_ROOT;
    int i, ret, num;
    struct mytype *tmp;

    if (argc < 2) {
	fprintf(stderr, "Usage: %s num\n", argv[0]);
	exit(-1);
    }

    num = atoi(argv[1]);

    printf("Please enter %d integers:\n", num);
    for (i = 0; i < num; i++) {
	tmp = malloc(sizeof(struct mytype));
	if (!tmp)
	    perror("Allocate dynamic memory");

	scanf("%d", &tmp->num);//
	
	ret = my_insert(&mytree, tmp);
	if (ret < 0) {
	    fprintf(stderr, "The %d already exists.\n", tmp->num);
	    free(tmp);
	}
    }

    printf("\nthe first test\n");
    print_rbtree(&mytree);

    my_delete(&mytree, 21);

    printf("\nthe second test\n");
    print_rbtree(&mytree);

    return 0;
}

