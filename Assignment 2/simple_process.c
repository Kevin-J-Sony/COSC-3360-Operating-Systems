#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "simple_process.h"

/*
*	This functions inserts a new instruction to the end of the queue. As input, it takes in the address to the queue, as well
*	as the instruction to be inserted.
*	This function receives the address to the pointer to the head of the linked list representing the queue. This is because
*	changes made to the pointer is not saved once the function returns, and so changes made to the pointer to the queue must be done
*	by a pointer pointing to it.
*/
void enqueue_instruction(struct instruction** proc_instruct, char* new_instruct) {
	struct instruction* new_node = (struct instruction*)malloc(sizeof(struct instruction));
	new_node->instruction = strdup(new_instruct);	// avoid complications by duping the string
	new_node->next_instruction = NULL;
	if (*proc_instruct == NULL) {
		*proc_instruct = new_node;
	} else {
		struct instruction* last_element = *proc_instruct;
		while (last_element->next_instruction != NULL)
			last_element = last_element->next_instruction;
		last_element->next_instruction = new_node;
	}
}

/*
*	This functions removes an instruction at the front of the queue, and returns it. As input, it takes in the address to a pointer
*/
char* deque_instruction(struct instruction** proc_instruct) {
	if (*proc_instruct == NULL) {
		return NULL;
	}
	struct instruction* head = *proc_instruct;
	char* cur_instruct = (*proc_instruct)->instruction;
	*proc_instruct = head->next_instruction;
	free(head);
	return cur_instruct;
}

/*
*	This function receives a pointer to the pointer to the head of the linked list representing the stack. This is because
*	changes made to the pointer is not saved once the function returns, and so changes made to the pointer to the stack must be done
*	by a pointer pointing to it.
*/
void push_resource_og(struct resource** resources, char* name) {
	struct resource* new_head = (struct resource*)malloc(sizeof(struct resource));
	new_head->resource_name = strdup(name);		// avoid complications by duping the string
	if (*resources != NULL)
		new_head->next_resource = *resources;
	*resources = new_head;
}

/*
*	This function receives an array of stacks, as well as the index of the array, and pushes the resource onto the stack responsible
*/
void push_resource(struct resource** resources, char* name, int type) {
	push_resource_og(&resources[type], name);
}


/*
*	This function receives a pointer to the pointer for the same reason as the other function. It returns a string; however, the real
*	purpose of it is to simply get rid of a resource. The name and type of a resource should already be read before popping.
*/
char* pop_resource_og(struct resource** resources) {
	if (*resources == NULL) {
		return NULL;
	}
	struct resource* top = *resources;
	char* resource_string = top->resource_name;
	*resources = top->next_resource;
	free(top);
	return resource_string;
}

/*
*	This function receives an array of stacks, as well as the index of the array, and pushes the resource onto the stack responsible
*/
char* pop_resource(struct resource** resources, int type) {
	return pop_resource_og(&resources[type]);
}