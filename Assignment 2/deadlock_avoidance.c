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
#include <errno.h>

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
	struct resource* allocated_resrcs;			// this will be read like a stack
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

/*
*	This function reads the instruction strings and returns the type of string it is encoded as an integer
*	It returns:
*		- 1 if instruction is calculate
*		- 2 if instruction is request
*		- 3 if instruction is use_resource
*		- 4 if instruction is release
*		- 5 if instruction is print_resources_used
*		- 6 if instruction is end
*/
int parse_instruction(char* instr_str) {
	// if (instr_str) {}
	return 3;
}

int safe_state(int n, int m, int* available, int** max, int** allocation) {
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
					flag2 = flag2 && (max[i][j] - allocation[i][j]) <= work[j];
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
	*	RESOURCES is an array of stacks not allocated to any process. When a process requests a specific resource,
	*	it will go to the stack at the specified index and pop the top, pushing it to the RESOURCE stack held by the process.
	*	It's important for the m to match the number of resources inside the sample_words.txt. Otherwise, there will be errors.
	*
	*	REQUEST is an array to be filled by each process whenever it requests resources.
	*/
	int* available = (int*)malloc(sizeof(int) * m);
	int* request = (int*)malloc(sizeof(int) * m);
	int** max = (int**)malloc(sizeof(int*) * n);
	int** allocation = (int**)malloc(sizeof(int*) * n);
	struct process* processes = (struct process*)malloc(sizeof(struct process) * n);
	struct resource** resources = (struct resource**)malloc(sizeof(struct resource*) * m);

	for (int i = 0; i < n; i++) {
		max[i] = (int*)malloc(sizeof(int) * m);
		allocation[i] = (int*)malloc(sizeof(int) * m);
		processes[i].proc_instructions = NULL;
		processes[i].allocated_resrcs = NULL;
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
	}

	/*
	*	There should only need to be one instance of struct sembuf since 
	*	this is a data structure to send information to the OS to change the internal
	*	state of the semaphore rather than modify the state directly.
	*
	*	There is m semaphores corresponding to the number of resources, as well as 1 extra
	*	semaphore so that the LLF scheduler and EDL scheduler do not run concurrently.
	*/
	struct sembuf op;
	int sid1 = semget(20, m + 1, 0666 | IPC_CREAT);
	int sid2 = semget(21, 1, 0666 | IPC_CREAT);
	
	if (sid1 == -1) {
        fprintf(stderr, "semget failed: %s\n", strerror(errno));
	}

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
		semop(sid1, &op, 1);
	}
	op.sem_num = m;
	op.sem_op = 1;
	op.sem_flg = 0;
	semop(sid1, &op, 1);

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
			processes[proc_count].remaining_computation_time = processes[proc_count].computation_time;

			int terminate = (1 == 0);
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
	
	char temp_string_example[] = "request(1,2,3,4);";
	char* tok = strtok(temp_string_example, "(,);\n");
	tok = strtok(NULL, "(,);\n");
	int idx = 0;
	int t;
	printf("request: ");
	while (tok != NULL) {
		sscanf(tok, "%d", &t);
		tok = strtok(NULL, "(,);\n");
		printf("%d\t", t);
	}
	printf("\n");

	/*
	*	Split the program into two, so that the program has two copies of processes and instructions.
	*	One part runs the Banker's algorithm with an EDL scheduler, and the other runs the Banker's algorithm
	*	with a LLF scheduler.
	*
	*	These will not run consecutively, so that the semaphores can work properly.
	*/
	int split = fork();
	int command_id = 0; // the seperate processes will assume the mantle of a commanding process

	if (split) {
		op.sem_num = m;	// the last semaphore
		op.sem_op = -1;
		op.sem_flg = 0;
		semop(sid2, &op, 1);

		FILE* output = fopen("temp1.txt", "w+");
		printf("The top code executed first\n");
		
		// check if the system is initially in a safe state
		int safe = safe_state(n, m, available, max, allocation);

		// create empty master string
		char* master_string = "";

		// create two semaphore to create mutex between child process and command process
		int sid3 = semget(22, 2, 0666 | IPC_CREAT);

		// Create n processes
		int process_id = 0;
		int tpid = 1;
		for (int i = 1; i <= n; i++) {
			if (tpid) {
				tpid = fork();
				if (!tpid) {
					process_id = i;
				}
			}
		}
		int time = 0;
		int instr_time;
		if (!process_id) {
			// when a process makes a request, it sends this request to the command processss
			// when it releases resources, it also sends the resources back to the command process
			int* finish = (int*) malloc(sizeof(int) * n);
			for (int i = 0; i < n; i++) {
				finish[i] = FALSE;
			}
			int flag = FALSE;
			
			char writing_process_id_str[5];
			int code;

			while (!flag) {
				// wait until process signals for sid3
				op.sem_num = 0;
				op.sem_op = -1;
				op.sem_flg = 0;
				semop(sid3, &op, 1);
	
				// figure out which process
				int child_proc_id;

				// read from the pipe dedicated for command process
				read((fd[0])[0], line, len);
				read((fd[0])[0], writing_process_id_str, 5);
				sscanf(writing_process_id_str, "%d", &child_proc_id);

				// read the first integer from line (code for method)
				char* tok = strtok(line, " \n");
				tok = strtok(NULL, " ");
				sscanf(tok, "%d", &code);

				if (code == 0) {
					// if code is 0, the child process is making a request
					for (int i = 0; i < m; i++) {
						tok = strtok(NULL, " ");
						sscanf(tok, "%d", &request[i]);
					}

					safe_state(n, m, available, max, allocation);

				} else if (code == 1) {
					// if code is 1,

				} else if (code == 2) {
					// if code is 2, 

				}

				write((fd[child_proc_id])[1], line, len);
				// furthemore, write down the process it came from
				write((fd[child_proc_id])[1], writing_process_id_str, 5);

				// signal that commanding process has finished execution
				op.sem_num = 1;
				op.sem_op = 1;
				op.sem_flg = 0;
				semop(sid3, &op, 1);
				
			}

		} else {
			int proc_id = process_id - 1;
			struct process curr_proc = processes[proc_id];
			char* string = deque_instruction(&curr_proc.proc_instructions);
			while (string != NULL) {
				int n = parse_instruction(string);
				switch(n) {
					// 1 is calculate
					case 1:
						sscanf(string, "calculate(%d)", &instr_time);
						time += instr_time;
						break;
					
					// 2 is request
					// the assignment demands that after each request, print the state of the process
					case 2:
						// load the request array
						char* tok = strtok(string, "(,);\n");
						tok = strtok(NULL, "(,);\n");
						int idx = 0;
						while (tok != NULL) {
							sscanf(tok, "%d", &request[idx++]);
							tok = strtok(NULL, "(,);\n");
						}
						// send it to the commanding process

						// read the commanding process command


						break;

					// 3 is use_resource
					case 3:

						break;

					// 4 is release
					case 4:
						break;

					// 5 is print_resources_used
					// print process number and master string
					case 5:
						break;

					// 6 is end
					case 6:
						break;
					
					default:
						fprintf(stderr, "Wrong n returned\n");
						exit(1);
				}

				string = deque_instruction(&curr_proc.proc_instructions);
			}
		}

		fclose(output);
		semctl(sid3, 0, IPC_RMID, 0);

		op.sem_num = m;	// the last semaphore
		op.sem_op = 1;
		op.sem_flg = 0;
		semop(sid2, &op, 1);

	} else {
		op.sem_num = m;	// the last semaphore
		op.sem_op = -1;
		op.sem_flg = 0;
		semop(sid2, &op, 1);

		FILE* output = fopen("temp2.txt", "w");
		printf("The bottom code executed\n");

		// check if the system is initially in a safe state
		int safe = safe_state(n, m, available, max, allocation);

		// create empty master string
		char* master_string = "";

		// create semaphore to restrict access to command process
		int sid3 = semget(22, 1, 0666 | IPC_CREAT);

		// Create n processes
		int process_id = 0;
		int tpid = 1;
		for (int i = 1; i <= n; i++) {
			if (tpid) {
				tpid = fork();
				if (!tpid) {
					process_id = i;
				}
			}
		}

		int time = 0;
		if (!process_id) {

		} else {
			int proc_id = process_id - 1;
			struct process curr_proc = processes[proc_id];
			char* string = deque_instruction(&curr_proc.proc_instructions);
			while (string != NULL) {
				// fprintf(stdout, "%s\n", string);
				string = deque_instruction(&curr_proc.proc_instructions);
			}
		}

		fclose(output);
		semctl(sid3, 0, IPC_RMID, 0);

		op.sem_num = m;	// the last semaphore
		op.sem_op = 1;
		op.sem_flg = 0;
		semop(sid2, &op, 1);
	}

	for (int i = 0; i < n; i++) {
		free(max[i]);
		free(allocation[i]);
	}
	free(max);
	free(available);
	free(request);
	free(allocation);

	semctl(sid1, 0, IPC_RMID, 0);
	semctl(sid2, 0, IPC_RMID, 0);


	return 0;
}

