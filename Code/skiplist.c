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
	//printf("node of level %i\n", level);
	node_array = malloc(level*sizeof(Node*));
	if (!node_array){
		fprintf(stderr, "Failed to allocated memory for a new node array\n");
	}
	
	for (int i = 0; i < level; i++){
		node_array[i] = malloc(sizeof(Node));
	}
	
	for (int i = 0; i < level; i++){
		node_array[i]->value = value;
		node_array[i]->node_level = level;
	}
	return node_array;
	
}


void delete_node_array(Node*** ptrToArrayOfPtrNode, SkipList* l){
	Node** to_delete = *ptrToArrayOfPtrNode;
	// iterate through each level of the node array of to_delete
	int node_level = to_delete[0]->node_level;
	for (int i = 0; i < node_level; i++){
		Node** prev_node = (to_delete[i]->prev);
		Node** next_node = (to_delete[i]->next);
		prev_node[i]->next = next_node;
		next_node[i]->prev = prev_node;
	}
	for (int i = 0; i < node_level; i++)
	{
		free(to_delete[i]);
	}
	free(to_delete);
	*ptrToArrayOfPtrNode = NULL;
	l->size -=1;

}

void skiplist_delete(SkipList** d) {
	SkipList* l = *d;
	Node** my_sentinel = l->sentinel;
	while (my_sentinel[0]->next[0]->value != -1){
		Node** to_delete = my_sentinel[0]->next;
		// iterate through each level of the node array of to_delete
		int node_level = to_delete[0]->node_level;
		for (int i = 0; i < node_level; i++){
			Node** prev_node = (to_delete[i]->prev);
			Node** next_node = (to_delete[i]->next);
			prev_node[i]->next = next_node;
			next_node[i]->prev = prev_node;
		}
		for (int i = 0; i < node_level; i++){
			free(to_delete[i]);
		}
		free(to_delete);
		
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


bool node_has_no_follower(Node** node){
	int node_level = node[0]->node_level;
	for (int i = 0; i < node_level; i++){
		if (node[i]->next[i]->value != -1){
			return false;
		}
	}
	return true;
}

Node** find_prev_node_to_insert(Node** node, int highest_level, int val_to_insert, unsigned int *nboperations){
	int next_value_of_highest_level = node[highest_level]->next[highest_level]->value;
	if (next_value_of_highest_level != -1)
	{
		*nboperations +=1;
	}
	
	if (next_value_of_highest_level >= val_to_insert){
		if (highest_level != 0){
			return find_prev_node_to_insert(node, highest_level-1, val_to_insert, nboperations);
		}else{
			return node;}
	}
	else{   
		if (node_has_no_follower(node)){// all level points to sentinel
			return node;
		}
		// If highest level > 0 and  points to sentinel THEN search lower level
		if (highest_level > 0 && next_value_of_highest_level == -1){
			return find_prev_node_to_insert(node, highest_level-1,val_to_insert, nboperations);
		}
		// If highest level does not point to sentinel
		else{ 
			return find_prev_node_to_insert(node[highest_level]->next, highest_level, val_to_insert, nboperations);
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
	
	//Node** new_prev = node_to_insert[insert_level]->prev;
	//Node** new_next = node_to_insert[insert_level]->next;
	//printf("value[%i] level [%i]->next : %i\n", node_to_insert[0]->value, insert_level, new_next[insert_level]->value);
	//printf("value[%i] level [%i]->prev : %i\n", node_to_insert[0]->value, insert_level, new_prev[insert_level]->value);

	//printf("value_prev[%i] level [%i]->next : %i\n", prev_node[0]->value, insert_level, (prev_node[insert_level]->next)[insert_level]->value);
	//printf("value_prev[%i] level [%i]->prev : %i\n", node_to_insert[0]->value, insert_level, new_prev[insert_level]->value);
}

void bind_arrays_of_nodes(Node**prev_node, Node** node_to_insert){
	int level = node_to_insert[0]->node_level;
	for (int i = 0; i < level; i++){
		//printf("Binding level %i\n", i);
		if (i <= prev_node[0]->node_level-1){
			Node** next_node = prev_node[i]->next;
			bind_nodes(prev_node, node_to_insert, next_node, i);
		}
		else{
			int current_max_level = prev_node[0]->node_level-1;
			while (current_max_level < i)
			{
				prev_node = prev_node[i-1]->prev;
				current_max_level = prev_node[0]->node_level-1;
			}
			Node** next_node = prev_node[i]->next;
			bind_nodes(prev_node, node_to_insert, next_node, i);
		}
	}
}

bool node_of_same_value(Node** node1, Node** node2){
	return (node1[0]->value == node2[0]->value);
}

SkipList* skiplist_insert(SkipList* d, int value) {
	Node** new_node = node_array_create(d, value);
	unsigned int search_number = 0;
	unsigned int* nboperations = &search_number;
	Node** prev_node_to_insert = find_prev_node_to_insert(d->sentinel, d->max_level-1, value, nboperations);
	// Case duplication
	if (prev_node_to_insert[0]->next[0]->value == value){
		Node** duplicate_node = prev_node_to_insert[0]->next;
		delete_node_array(&duplicate_node, d);
	}
	bind_arrays_of_nodes(prev_node_to_insert, new_node);
	d->size +=1;
	return d;
}


bool skiplist_search(const SkipList* d, int value, unsigned int *nb_operations){
	Node** sentinel = d->sentinel;
	Node** biggest_prev_node = find_prev_node_to_insert(sentinel, d->max_level-1, value, nb_operations);
	if (biggest_prev_node[0]->next[0]->value == value){
		return true;
	}
	return false;
}