#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
	int size;
};
 
SkipList* skiplist_create(int nblevels) {
	//nblevels = rng_initialize(0, nblevels);
	SkipList* l;
	l = malloc(sizeof(SkipList)+nblevels*sizeof(Node)); 
	l->sentinel = (Node**)(l+1);
	l->size = 0;
	for (int i = 0; i < nblevels; i++){
		l->sentinel[i]->next = l->sentinel;
		l->sentinel[i]->prev = l->sentinel;
	}
	return l;
}

Node** node_array_create(int value, unsigned int level){
	Node** node_array;
	node_array = malloc(level*sizeof(Node));
	for (unsigned int i = 0; i < level-1; i++)
	{
		node_array[i]->value = value;
		node_array[i]->node_level = level;
	}
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
		int node_level = element[0]->node_level;
		for (int i = 0; i < node_level ; i++){
			f(element[i]->value, user_data);
		}
	}
}


Node** find_next_node(Node**sentinel, Node** node, int* current_level){
	Node** next_node;
	// if there is a next node
	if (node[*current_level]->next != sentinel){
		next_node = node[*current_level]->next;
	}
	// if node points to sentinel and node level != 0
	// return first node of a lower level
	if ((node[*current_level]->next == sentinel)&&*current_level>0)
	{
		*current_level -= 1;
		next_node = sentinel[*current_level]->next;
	}
	else{ // reaching the end of the list at level 0
		next_node = node;
	}
	return next_node;
}

Node** find_prev_node_to_insert(SkipList*d, int value){
	int level = d->max_level - 1;
	int* current_level = &level; // array of n elements has maximum index of n-1
	Node** prev_node_to_insert = NULL;
	Node** sentinel = d->sentinel;
	Node** current_node = sentinel[*current_level]->next;
	int node_value = current_node[0]->value;
	while (value < node_value && *current_level != 0){
		current_node = find_next_node(sentinel, current_node, current_level);
		node_value = current_node[0]->value;
	}
	// Insert to the beginning of the list
	if (value < node_value && *current_level == 0){
		return sentinel;
	}
	else{
		int current_level = current_node[0]->node_level;
		prev_node_to_insert = current_node;
		for (int i = 0; i < current_level; i++){
			if ((prev_node_to_insert[i]->next)[0]->value < value)
			{
				prev_node_to_insert = prev_node_to_insert[i]->next;
			}
			else{
				break;
			}	
		}
	}
	return prev_node_to_insert;
}

void bind_nodes(Node** prev_node, Node** node_to_insert, Node** next_node, int insert_level){
	//bind to the prev node
	prev_node[insert_level]->next = node_to_insert;
	node_to_insert[insert_level]->prev = prev_node;
	//bind to next node
	node_to_insert[insert_level]->next = next_node;
	next_node[insert_level]->prev = node_to_insert;
}

SkipList* skiplist_insert(SkipList* d, int value) {
	
	int max_level = d->max_level;
	RNG rng = rng_initialize(123456789ULL, max_level);
	unsigned int random_level = rng_get_value(&rng)+1;

	Node** new_node = node_array_create(value, random_level);

	Node** prev_node_to_insert = find_prev_node_to_insert(d, value);

	if(random_level <= (unsigned int) prev_node_to_insert[0]->node_level){
		for (unsigned int i = 0; i < random_level; i++){
			Node** next_node = prev_node_to_insert[i]->next;
			bind_nodes(prev_node_to_insert,new_node,next_node, i);
		}
	}
	else{
		for (unsigned int i = 0; i < random_level; i++){
			if ((unsigned int) prev_node_to_insert[0]->node_level <= random_level-1){
				Node** next_node = prev_node_to_insert[i]->next;
				bind_nodes(prev_node_to_insert,new_node,next_node, i);
			}
			else{
				prev_node_to_insert = prev_node_to_insert[i-1]->prev;
				while ((unsigned int) prev_node_to_insert[0]->node_level <= random_level ){
					prev_node_to_insert = prev_node_to_insert[i-1]->prev;
				}
				Node** next_node = prev_node_to_insert[i]->next;
				bind_nodes(prev_node_to_insert,new_node,next_node, i);
			}
		}
		
	}
	d->size +=1;
	return d;
}