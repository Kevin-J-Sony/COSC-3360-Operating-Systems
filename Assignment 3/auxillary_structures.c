#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "auxillary_structures.h"

/*
*	This functions inserts a new instruction to the end of the list. As input, it takes in a pointer to the pointer to the head of the list, as well
*	as the instruction to be inserted.
*
*   Return the head of the linked list
*/
void add_instruction(struct instruction** list_head, int pid, int addr) {
	// create a new node to be inserted
	struct instruction* new_node = (struct instruction*)malloc(sizeof(struct instruction));
	new_node->pid = pid;
	new_node->addr = addr;
	new_node->next_instruction = NULL;

	// if the linked list head is null, set it to be
	if (*list_head == NULL) {
		*list_head = new_node;
	} else {
		struct instruction* last_element = *list_head;
		while (last_element->next_instruction != NULL)
			last_element = last_element->next_instruction;
		last_element->next_instruction = new_node;
	}
}


/*
*   This function takes in a pointer to a linked list and frees all of the nodes in the list
*/
void destroy_instruction_list(struct instruction* list_head) {
	struct instruction* node = list_head;
	while (node != NULL) {
		node = node->next_instruction;
		free(list_head);
		list_head = node;
	}
}

