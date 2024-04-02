/*
*	This program simulates deadlock avoidance using the Banker's algorithm
*   combined with Earliest-Deadline-First scheduling (using Longest-Job-First as tie-breaker)
*   and Least-Laxity-First scheduing (using Shortest-Job-First as tie-breaker). 
*   
*   It is an experiment to determine which scheduling works best with the Banker's Algorithm
*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TRUE (1 == 1)
#define FALSE (1 == 0)

/*
*	Create a queue for instructions. The elements are added from the back of the linked list, and removed
*	from the front of the linked list. The reason for this is because 
*/
struct instruction {
	char* instruction;
	struct instruction* next_instruction;
};

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

struct process {
	int proc_id;
	int deadline;
	int computation_time;

	struct instruction* proc_instructions;		// this will be read like a queue
	int current_idx;	// during simulation, current_idx points to the current instruction running
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
*	This function receives a pointer to the pointer to the head of the linked list representing the stack. This is because
*	changes made to the pointer is not saved once the function returns, and so changes made to the pointer to the stack must be done
*	by a pointer pointing to it.
*/
void push_resource(struct resource** resources, char* name) {
	struct resource* new_head = (struct resource*)malloc(sizeof(struct resource));
	new_head->resource_name = strdup(name);		// avoid complications by duping the string
	if (*resources != NULL)
		new_head->next_resource = *resources;
	*resources = new_head;
}

/*
*	This function receives a pointer to the pointer for the same reason as the other function. 
*	returns a pointer to the new head of the stack.
*/
char* pop_resource(struct resource** resources) {
	if (*resources == NULL) {
		printf("ERROR: POPPING EMPTY STACK\n");
		return NULL;
	}
	struct resource* top = *resources;
	char* resource_string = top->resource_name;
	*resources = top->next_resource;
	free(top);
	return resource_string;
}

int safe_state(int n, int m, int* available, int** need, int** allocation) {
	// need + allocation = max
	int* work = (int*)malloc(sizeof(int) * m);
	int* finish = (int*)malloc(sizeof(int) * n);
	for (int i = 0; i < m; i++) {
		work[i] = available[i];
	}
	for (int i = 0; i < n; i++) {
		finish[i] = FALSE;
	}
	int flag = FALSE;
	while (!flag) {
		int flag3 = FALSE;
		for (int i = 0; i < n; i++) {
			if (!finish[i]) {
				flag3 = TRUE;
				int flag2 = TRUE;
				for (int j = 0; j < m; j++) {
					flag2 = flag2 && need[i][j] <= work[j];
				}
				if (flag2) {
					for (int j = 0; j < m; j++) {
						work[j] = work[j] + allocation[i][j];
					}
					finish[i] = TRUE;
				}
			}
		}
		// if flag3 is not flagged at all, it would remain false and so set flag to true
		flag = !flag3;
	}

	for (int i = 0; i < n; i++) {
		if (!finish[i]) {
			return FALSE;
		}
	}
	return TRUE;
}


/*
*	This program will simulate how the Banker's algorithm will allocate resources to processes.
*	The algorithm will use the Earliest-Deadline-First scheduler (with Longest-Job-First as tie breaker) and 
*	the Least-Laxity-First scheduler (with Shortest-Job-First as tie breaker).
*
*	Two different schedulers mean that two different outputs are given.
*/
int main(int argc, char** argv) {
	/*
	*   There is a single argument provided as input. It has two integers, m and n,
	*   in the first two lines, signifying the number of resource types and the number
	*   of processes respectively. While the assignment specifies two different input formats
	*   developing the program to account for different formats is unnecessarily complicated.
	*   Thus the next m and n*m lines are integers to be filled into the AVAILABLE array and MAX
	*   matrix respectively.
	* 
	*   Afterwards, the following lines contain instructions to simulate how the processes will use
	*   the resources.
	* 
	*   The output is the execution of the processes with the EDL scheduler and the LLF scheduler.
	*	Since two different schedulers are used, output should reflect that
	*/
	int n, m;
	FILE* input = fopen(argv[1], "r");

	/*
	*   If the files are not provided, exit the program
	*/
	if (input == NULL) {
		printf("The file is null\n");
		exit(EXIT_FAILURE);
	}

	/*
	*	The lines will not exceed 300 characters, so allocate 300 characters as a buffer.
	*	Read the first two integers, initialize the AVAILABLE and MAX data structures, as well as
	*   any other data structures necessary, and then read the next line.
	*/
	size_t len = 300;
	char line[len];
	char sec_line[len];
	fgets(line, len, input);
	sscanf(line, "%d", &m);
	fgets(line, len, input);
	sscanf(line, "%d", &n);

	/*
	*	MAX and AVAILABLE are not stored within the process/resource data structure for ease of the 
	*	safety algorithm
	*
	*	RESOURCES is an array of stacks. It's important for the m to match the number of resources
	*	inside the sample_words.txt. Otherwise, there will be errors.
	*/
	int* available = (int*)malloc(sizeof(int) * m);
	int** max = (int**)malloc(sizeof(int*) * n);
	int** allocation = (int**)malloc(sizeof(int*) * n);
	struct process* processes = (struct process*)malloc(sizeof(struct process) * n);
	struct resource** resources = (struct resource**)malloc(sizeof(struct resource*) * m);

	for (int i = 0; i < n; i++) {
		max[i] = (int*)malloc(sizeof(int) * m);
		allocation[i] = (int*)malloc(sizeof(int) * m);
		processes[i].proc_instructions = NULL;
	}


	for (int i = 0; i < m; i++) {
		fscanf(input, "%d", &available[i]);
		resources[i] = (struct resource*)NULL;
	}
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			fscanf(input, "%d", &max[i][j]);
			allocation[i][j] = 0;
		}
		printf("\n");
	}

	/*
	*	There should only need to be one instance of struct sembuf since 
	*	this is a data structure to send information to the OS to change the internal
	*	state of the semaphore rather than modify the state directly.
	*/
	struct sembuf op;
	int sid = semget(2077318, m, 0666 | IPC_CREAT);
	/*
	*	In allocating a resource, the following instructions are used
	*		op.sem_num = resource_idx;
	*		op.sem_op = -1;
	*		op.sem_flg = 0;
	*		semop(sid, &op, 1);
	*	In releasing a resource, the following instructions are executed
	*		op.sem_num = resource_idx;
	*		op.sem_op = -1;
	*		op.sem_flg = 0;
	*		semop(sid, &op, 1);
	*/
	// the values in the semaphore will correspond to the values in AVAILABLE
	for (int i = 0; i < m; i++) {
		op.sem_num = i;
		op.sem_op = available[i];
		op.sem_flg = 0;
		semop(sid, &op, 1);
	}

	int current_execution_time = 0;

	/*
	*	The remaining lines in the file are the processes, their deadline and computation time,
	*	and their instructions. First read the process line; the next two lines indicate deadline,
	*	and computation time. The following lines are instructions that the process should execute as
	*	well as an instruction to print the resources used up so far. Finally, the last instruction
	*/
	int proc_count = 0;
	while (!feof(input)) {
		fgets(line, len, input);
		strncpy(sec_line, line, strlen("process"));
		if (strcmp("process", sec_line) == 0) {
			// we have the start to a process
			// the next two lines give the deadline and computation time
			processes[proc_count].proc_id = proc_count;
			fgets(line, len, input);
			sscanf(line, "%d", &processes[proc_count].deadline);
			fgets(line, len, input);
			sscanf(line, "%d", &processes[proc_count].computation_time);

			int terminate = (1 == 0);
			printf("PROCESS COUNT: %d\n", proc_count);
			while (!feof(input) && !terminate) {
				fgets(line, len, input);
				strcpy(sec_line, line);
				if (strcmp("end.\n", sec_line) == 0) {
					terminate = (1 == 1);
				} else {
					// store the instructions in the vector					
					enqueue_instruction(&processes[proc_count].proc_instructions, line);
				}
				memset(sec_line, 0, sizeof(sec_line));				
			}
			proc_count++;
		}
		memset(sec_line, 0, sizeof(sec_line));
	}
	fclose(input);

	// test if instruction queue works
	/*
	printf("INSTRUCTION OUTPUT\n\n");
	for (int i = 0; i < n; i++) {
		printf("PROCESS %d\n", i);
		char* instruction = deque_instruction(&processes[i].proc_instructions);
		while(instruction != NULL) {
			printf("%s\t", instruction);
			instruction = deque_instruction(&processes[i].proc_instructions);
		}
	}*/

	input = fopen(argv[2], "r");
	
	/*
	*   If the file is not provided, exit the program
	*/
	if (input == NULL) {
		printf("The file is null\n");
		exit(EXIT_FAILURE);
	}

	// read the resource name and type
	// somwhere in here, max freaks out
	int ridx = 0;
	while (!feof(input)) {
		fgets(line, len, input);
		char* tok = strtok(line, ": ,");
		tok = strtok(NULL, ": ,");
		tok = strtok(NULL, ": ,");
		while (tok != NULL) {
			printf("%s\n", tok);
			push_resource(&resources[ridx], tok);
			tok = strtok(NULL, ": ,\n");
		}
		ridx++;
	}
	fclose(input);

	// test if resource stack works
	/*
	for (int i = 0; i < m; i++) {
		char* resource = pop_resource(&resources[i]);
		while(resource != NULL) {
			printf("%s\t", resource);
			resource = pop_resource(&resources[i]);
		}
		printf("\n");
	}*/

	/*
	*	Split the program into two, so that the program has two copies of processes and instructions.
	*	One part runs the Banker's algorithm with an EDL scheduler, and the other runs the Banker's algorithm
	*	with a LLF scheduler.
	*/
	int split = fork();
	if (split) {
		FILE* output = fopen("temp1.txt", "w");
		
		fclose(output);
	} else {
		FILE* output = fopen("temp2.txt", "w");
		
		fclose(output);
	}

	// create n processes
	/*
	int process_id = 0;
	int tpid = 1;
	for (int i = 1; i <= n; i++) {
		if (tpid) {
			tpid = fork();
			if (!tpid) {
				process_id = i;
			}
		}
	}*/



	// test to see if input is read
	/*
	for (int i = 0; i < m; i++) {
		printf("%d\t", available[i]);
	}
	printf("\n\n");
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			printf("%d\t", max[i][j]);
		}
		printf("\n");
	}*/


	for (int i = 0; i < n; i++) {
		free(max[i]);
		free(allocation[i]);
	}
	free(max);
	free(available);
	free(allocation);


	return 0;
}

