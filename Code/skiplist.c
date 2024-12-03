#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "skiplist.h"
#include "rng.h"
typedef struct s_Node Node;
typedef struct s_Link Link;

struct s_Node{
	int value;
	Node** prev;
	Node** next;
	int node_level;
};

struct s_SkipList{
	Node** sentinel;
	int max_level;
	unsigned int size;
	RNG rng;
};
 
SkipList* skiplist_create(int nblevels) {
	//nblevels = rng_initialize(0, nblevels);
	SkipList* l;
	l = malloc(sizeof(SkipList)+nblevels*sizeof(Node*)); 
	if (!l)
	{
		fprintf(stderr, "Memory allocation failed for Skiplist,\n");
		exit(1);
	}
	l->max_level = nblevels;
	l->sentinel = (Node**)(l+1);
	l->size = 0;
	for (int i = 0; i < nblevels; i++){
		(l->sentinel)[i] = malloc(sizeof(Node));
	}
	for (int i = 0; i < nblevels; i++)
	{
		(l->sentinel)[i]->next = l->sentinel;
		l->sentinel[i]->prev = l->sentinel;
		l->sentinel[i]->value = -1;      
        l->sentinel[i]->node_level = nblevels; 
	}
	l->rng = rng_initialize(0x7FFFFFFF, nblevels);

	return l;
}

Node** node_array_create(SkipList*d, int value){
	Node** node_array;
	int level = rng_get_value(&d->rng) + 1;
	printf("node of level %i\n", level);
	node_array = malloc(level*sizeof(Node*));
	if (!node_array)
	{
		fprintf(stderr, "Failed to allocated memory for a new node array\n");
	}
	
	for (int i = 0; i < level; i++)
	{
		node_array[i] = malloc(sizeof(Node));
	}
	//printf("finished initializing nodes\n");
	
	for (int i = 0; i < level; i++)
	{
		node_array[i]->value = value;
		node_array[i]->node_level = level;
	}
	//printf("finished assigning values to nodes\n");

	return node_array;
	
}

void skiplist_delete(SkipList** d) {
	SkipList* l = *d;
	Node** my_sentinel = l->sentinel;
	while (my_sentinel[0]){
		Node** to_delete = my_sentinel[0]->next;
		// iterate through each node of the node array of to_delete
		for (int i = 0; i < to_delete[0]->node_level; i++){

			Node* current_node = to_delete[0];
			if (current_node->next)
			{
				Node* prev_node = (current_node->prev)[i];
				Node* next_node = (current_node->next)[i];
				prev_node->next = current_node ->next;
				next_node->prev = current_node->prev;
			}
		}
		int node_level = to_delete[0]->node_level;
		for (int i = 0; i < node_level; i++)
		{
			free(to_delete[i]);
		}
		
	}
	free(l);
	*d = NULL;

}

unsigned int skiplist_size(const SkipList *d){
	return d->size;
}

int skiplist_at(const SkipList *d, unsigned int i){
	Node** current_node = d->sentinel;

	for (unsigned int pos = 0; pos < i; pos++)
	{
		current_node = current_node[0]->next;
	}
	return current_node[0]->value;
}


void skiplist_map(const SkipList* d, ScanOperator f, void *user_data){
	Node** sentinel = d->sentinel;
	for(Node** element = sentinel[0]->next; element != d->sentinel; element = element[0]->next){
		f(element[0]->value, user_data);
	}
}




Node** find_next_node(Node**sentinel, Node** node, int* current_level){
	Node** next_node;
	// if there is a next node
	if (node[*current_level]->next != sentinel){
		printf("case 1\n");
		next_node = node[*current_level]->next;
	}
	// if node points to sentinel and node level != 0
	// return first node of a lower level
	if ((node[*current_level]->next == sentinel)&&*current_level>0)
	{
		printf("case 2\n");
		*current_level -= 1;
		next_node = sentinel[*current_level]->next;
	}
	else{ // reaching the end of the list at level 0
		printf("case 1\n");
		next_node = node;
	}
	return next_node;
}

void delete_node(Node*** ptrArrPtrNode){
	Node** node = *ptrArrPtrNode;
	int level = node[0]->node_level;
	for (int i = 0; i < level; i++)
	{
		free(node[i]);
	}
	free(node);
	*ptrArrPtrNode = NULL;
	
}

bool node_has_no_follower(Node** node){
	int node_level = node[0]->node_level;
	for (int i = 0; i < node_level; i++){
		if (node[i]->next[i]->value != -1){
			return false;
		}
	}
	return true;
}

Node** find_prev_node_to_insert(Node** node, int highest_level, int val_to_insert){
	printf("finding prev node\n");
	int next_value_of_highest_level = node[highest_level]->next[highest_level]->value;

	printf("next_value_of_highest_level = %i\n", next_value_of_highest_level);
	if (next_value_of_highest_level > val_to_insert){
		if (highest_level != 0){
			return find_prev_node_to_insert(node, highest_level-1, val_to_insert);
		}else{
			return node;}
	}
	else{   
		if (node_has_no_follower(node)){// all level points to sentinel
			return node;
		}
		// If highest level > 0 and  points to sentinel THEN search lower level
		if (highest_level > 0 && next_value_of_highest_level == -1){
			return find_prev_node_to_insert(node, highest_level-1,val_to_insert);
		}
		// If highest level does not point to sentinel
		else{ 
			return find_prev_node_to_insert(node[highest_level]->next, highest_level, val_to_insert);
		}
		
	}
}

void bind_nodes(Node** prev_node, Node** node_to_insert, Node** next_node, int insert_level){
	//bind to the prev node
	prev_node[insert_level]->next = node_to_insert;
	node_to_insert[insert_level]->prev = prev_node;
	//bind to next node
	node_to_insert[insert_level]->next = next_node;
	next_node[insert_level]->prev = node_to_insert;
	
	Node** new_prev = node_to_insert[insert_level]->prev;
	Node** new_next = node_to_insert[insert_level]->next;
	printf("value[%i] level [%i]->next : %i\n", node_to_insert[0]->value, insert_level, new_next[insert_level]->value);
	printf("value[%i] level [%i]->prev : %i\n", node_to_insert[0]->value, insert_level, new_prev[insert_level]->value);

	printf("value_prev[%i] level [%i]->next : %i\n", prev_node[0]->value, insert_level, (prev_node[insert_level]->next)[insert_level]->value);
	//printf("value_prev[%i] level [%i]->prev : %i\n", node_to_insert[0]->value, insert_level, new_prev[insert_level]->value);
}

void bind_arrays_of_nodes(Node**prev_node, Node** node_to_insert){
	int level = node_to_insert[0]->node_level;
	for (int i = 0; i < level; i++){
		printf("Binding level %i\n", i);
		if (i <= prev_node[0]->node_level-1)
		{
			Node** next_node = prev_node[i]->next;
			bind_nodes(prev_node, node_to_insert, next_node, i);
		}
		else{
			printf("Case prev_node shorter: %i\n", i);
			int current_max_level = prev_node[0]->node_level-1;
			while (current_max_level < i)
			{
				printf("current_max_level: %i\n", current_max_level);
				prev_node = prev_node[i-1]->prev;
				current_max_level = prev_node[0]->node_level-1;
			}
			printf("found prev_node %i has level == level of i : %i\n", prev_node[0]->value, i);
			Node** next_node = prev_node[i]->next;
			bind_nodes(prev_node, node_to_insert, next_node, i);
		}
	}
}


SkipList* skiplist_insert(SkipList* d, int value) {
	printf("max_level = %i\n", d->max_level);
	Node** new_node = node_array_create(d, value);
	printf("new node = %i \n", new_node[0]->value);
	Node** prev_node_to_insert = find_prev_node_to_insert(d->sentinel, d->max_level-1, value);
	printf("FOUND PREV NODE %i\n", prev_node_to_insert[0]->value);

	bind_arrays_of_nodes(prev_node_to_insert, new_node);
	d->size +=1;
	printf("***FINISHED	INSERT NODE %i****\n", value);
	return d;
}