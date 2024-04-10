

/*
*	Create a queue for instructions. The elements are added from the back of the linked list, and removed
*	from the front of the linked list. The reason for this is because 
*/
struct instruction {
	char* instruction;
	struct instruction* next_instruction;
};

/*
*	This is an enumeration of all the possible states of a process. An additional state is added to the possible states of a process
*	since this assignment demands that if a process makes a request, it must cede execution to another process until it makes an request or terminates
*/
enum process_state {
	CREATED,
	READY,
	RUNNING,
	WAITING,
	REQUESTED,
	FINISHED
};


/*
*	This data structure stores the information for the process. In this assignment, the assumption made is that the arrival time for all
*	processes are t=0
*/
struct process {
	int proc_id;
	int deadline;
	int computation_time;
	int remaining_computation_time;

	struct instruction* proc_instructions;		// this will be read like a queue
	struct resource** arr_allocated_resrcs; 	// this will be read like an array of stacks
    int* request_array;
	enum process_state state;

	char* master_str;
	int n_str_master;	// this gives the number of strings in master_str

	int deadline_misses;
};



/*
*	Resources are stored as a linked list which act as a stack. The stack grows from left to right, and the
*	top of the stack is given by the current head. Popping removes the head, replacing it with the element the head points to
*	and pushing adds a new head, pointing to the previous head
*/
struct resource {
	char* resource_name;
	struct resource* next_resource; 
};








/*
*	This functions inserts a new instruction to the end of the queue. As input, it takes in the address to the queue, as well
*	as the instruction to be inserted.
*	This function receives the address to the pointer to the head of the linked list representing the queue. This is because
*	changes made to the pointer is not saved once the function returns, and so changes made to the pointer to the queue must be done
*	by a pointer pointing to it.
*/
void enqueue_instruction(struct instruction** proc_instruct, char* new_instruct);

/*
*	This functions removes an instruction at the front of the queue, and returns it. As input, it takes in the address to a pointer
*/
char* deque_instruction(struct instruction** proc_instruct);

/*
*	This function receives a pointer to the pointer to the head of the linked list representing the stack. This is because
*	changes made to the pointer is not saved once the function returns, and so changes made to the pointer to the stack must be done
*	by a pointer pointing to it.
*/
void push_resource_og(struct resource** resources, char* name);

/*
*	This function receives an array of stacks, as well as the index of the array, and pushes the resource onto the stack responsible
*/
void push_resource(struct resource** resources, char* name, int type);

/*
*	This function receives a pointer to the pointer for the same reason as the other function. It returns a string; however, the real
*	purpose of it is to simply get rid of a resource. The name and type of a resource should already be read before popping.
*/
char* pop_resource_og(struct resource** resources);

/*
*	This function receives an array of stacks, as well as the index of the array, and pushes the resource onto the stack responsible
*/
char* pop_resource(struct resource** resources, int type);