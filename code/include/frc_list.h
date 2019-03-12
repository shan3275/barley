#ifndef __FRC_LIST_H__
#define __FRC_LIST_H__


struct __frc_list_node  {
	struct __frc_list_node *le_next;
	struct __frc_list_node *le_prev;
};

typedef struct __frc_list_node  frc_list_t;

static inline void
FRC_INIT_LIST_HEAD(frc_list_t *ptr)
{
	ptr->le_next = ptr; ptr->le_prev = ptr;
}



static inline
void frc_list_add_head(frc_list_t *node, frc_list_t *head)
{
	head->le_next->le_prev = node;
	node->le_next   = head->le_next;
	head->le_next   = node;
	node->le_prev   = head;
}


static inline
void frc_list_add_tail(frc_list_t *node, frc_list_t *head)
{
	head->le_prev->le_next = node;
	node->le_prev   = head->le_prev;
	head->le_prev   = node;
	node->le_next   = head;
}






/* Move nodes from list2 to list1. list1 must be empty. list2 will be empty
   when this call returns. */
static inline
void frc_list_move(frc_list_t *list1, frc_list_t *list2)
{
	if(list2->le_next != list2) {
		list1->le_next = list2->le_next;
		list1->le_next->le_prev = list1;
		list1->le_prev = list2->le_prev;
		list1->le_prev->le_next = list1;
	}

	list2->le_next = list2->le_prev = list2;
}




static inline
frc_list_t *frc_list_get_head(frc_list_t  *root)
{
	if( (root->le_prev == root)  &&  (root->le_next == root) )
		return NULL;

	return root->le_next;
}



static inline
frc_list_t *frc_list_get_tail(frc_list_t  *root)
{
	if( (root->le_prev == root)  &&  (root->le_next == root) )
		return NULL;

	return root->le_prev;
}





static inline 
void frc_list_del(frc_list_t *node)
{
	node->le_next->le_prev = node->le_prev;
	node->le_prev->le_next = node->le_next;
}




static inline
frc_list_t *frc_list_delete_head(frc_list_t  *root)
{
	frc_list_t  *node = frc_list_get_head(root);
	if(node)
		frc_list_del(node);

	return node;
}



#define frc_list_for_each(tmp, head)  \
	for (tmp = (head)->le_next; tmp != (head); tmp = tmp->le_next)


#define frc_list_for_each_safe(tmp, tmp2, head)  \
	for (tmp = (head)->le_next, tmp2 = tmp->le_next; tmp != (head); tmp = tmp2, tmp2 = tmp->le_next)


#endif /* !__FRC_LIST_H__ */


/* $Id: frc-list.h 45651 2009-10-30 21:43:29Z panicker $ */
