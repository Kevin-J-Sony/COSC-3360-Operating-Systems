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
*	This is an enumeration of all the possible states of a process. An additional state is added to the possible states of a process
*	since this assignment demands that if a process makes a request, it must cede execution to another process until it makes an request or terminates
*/
enum process_state {
	READY,
	RUNNING,
	WAITING,
	REQUESTED
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
	struct resource* allocated_resrcs;			// this will be read like a stack

	struct resource** arr_allocated_resrcs;			// this will be read like an array of stacks

	enum process_state state;

	char* master_str;
	int n_str_master;	// this gives the number of strings in master_str
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


/*
*	This function reads the instruction strings and returns the type of string it is encoded as an integer
*	It returns:
*		- 1 if instruction is calculate
*		- 2 if instruction is request
*		- 3 if instruction is use_resource
*		- 4 if instruction is release
*		- 5 if instruction is print_resources_used
*		- 6 if instruction is end.
*/
int parse_instruction(char* instr_str) {
	char* temp = strdup(instr_str);
	char* tok = strtok(temp, "(,);\n");
	printf("tok: %s\n", tok);
	tok = strtok(NULL, "(,);\n");
	
	if (strcmp(temp, "calculate") == 0) {
		return 1;
	} else if (strcmp(temp, "request") == 0) {
		return 2;
	} else if (strcmp(temp, "use_resources") == 0) {
		return 3;
	} else if (strcmp(temp, "release") == 0) {
		return 4;
	} else if (strcmp(temp, "print_resources_used") == 0) {
		return 5;
	} else if (strcmp(temp, "end.") == 0) {
		return 6;
	} else {
		printf("temp (%s)\n", temp);
		return -1;
	}
}

int safe_state(int n, int m, int* available, int** max, int** allocation, int* true_finish) {
	// need + allocation = max
	int* work = (int*)malloc(sizeof(int) * m);
	int* finish = (int*)malloc(sizeof(int) * n);
	for (int i = 0; i < m; i++) {
		work[i] = available[i];
	}
	// get the state of all the processes so far
	for (int i = 0; i < n; i++) {
		finish[i] = true_finish[i];
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
	free(finish);
	free(work);
	return TRUE;
}

/*
*	Comparator function for qsort in master_string
*/
int alphabetical_order_comp(const void* p1, const void* p2) {
	return strcmp((char*)p1, (char*)p2);
}

/*
*	This function takes in the string stored in p_process made from "use_resources", and outputs the master string
*	for this process
*/
char* print_master_string(struct process* p_process) {
	// we first get the number of strings outputted
	int size_str_arr = p_process->n_str_master;

	// we allocate an array of strings
	char** total_str = (char**) malloc(sizeof(char*) * size_str_arr);
	
	// next get all of the strings from master_str
	char* temp = strdup(p_process->master_str);
	char* tok = strtok(temp, " ");
	for (int i = 0; i < size_str_arr; i++) {
		total_str[i] = strdup(tok);
		tok = strtok(NULL, " ");
	}

	// next, sort the array alphebetically
	qsort(total_str, size_str_arr, sizeof(char*), alphabetical_order_comp);

	// finally, output the string
	char* final_string = strdup("");
	for (int i = 0; i < size_str_arr; i++) {
		strcat(final_string, total_str[i]);
	}

	return strdup(final_string);
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
	*	The lines will not exceed 1000 characters, so allocate 1000 characters as a buffer.
	*	Read the first two integers, initialize the AVAILABLE and MAX data structures, as well as
	*   any other data structures necessary, and then read the next line.
	*/
	size_t len = 1000;
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
	*
	*	FINISH is an array that indicates whether process at an index is finished
	*/
	int* available = (int*)malloc(sizeof(int) * m);
	int* request = (int*)malloc(sizeof(int) * m);
	int** max = (int**)malloc(sizeof(int*) * n);
	int** allocation = (int**)malloc(sizeof(int*) * n);
	struct process* processes = (struct process*)malloc(sizeof(struct process) * n);
	struct resource** resources = (struct resource**)malloc(sizeof(struct resource*) * m);

	int* finish = (int*)malloc(sizeof(int) * n);


	for (int i = 0; i < n; i++) {
		max[i] = (int*)malloc(sizeof(int) * m);
		allocation[i] = (int*)malloc(sizeof(int) * m);
		finish[i] = FALSE;
		processes[i].proc_instructions = NULL;
		processes[i].allocated_resrcs = NULL;
		processes[i].master_str = strdup("");
		processes[i].state = WAITING;
		processes[i].arr_allocated_resrcs = (struct resource**)malloc(sizeof(struct resource*) * m);
 	}


	for (int i = 0; i < m; i++) {
		fscanf(input, "%d", &available[i]);
		resources[i] = (struct resource*)NULL;
		for (int j = 0; j < n; j++) {
			processes[i].arr_allocated_resrcs[i] = (struct resource*) NULL;
		}
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
			push_resource(resources, tok, ridx);
			tok = strtok(NULL, ": ,\n");
		}
		ridx++;
	}
	fclose(input);

	// test if resource stack works
	printf("First array: %s\n", resources[0]->resource_name);
	printf("second array: %s\n", resources[0]->next_resource->resource_name);
	
	for (int i = 0; i < m; i++) {
		// char* resource = pop_resource(&resources[i]);
		char* resource = pop_resource(resources, i);
		while(resource != NULL) {
			printf("%s\t", resource);
			// resource = pop_resource(&resources[i]);
			resource = pop_resource(resources, i);
			
		}
		printf("\n");
	}
	
	// test if string can be parsed
	/*
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
	*/

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
		// execute EDL and LLF seperatly
		op.sem_num = m;
		op.sem_op = -1;
		op.sem_flg = 0;
		semop(sid2, &op, 1);

		FILE* output = fopen("temp1.txt", "w+");

		// check if the system is initially in a safe state
		int safe = safe_state(n, m, available, max, allocation, finish);

		// create empty master string
		char* master_string = "";

		// create n + 1 semaphores, one for each child process, and one for deadlock process
		// parent process has process_id = n, so the semaphore corresponding to it is semaphore @ n
		int sid3 = semget(22, n + 1, 0666 | IPC_CREAT);

		// create n + 1 file descriptors, one for each child process, and one for deadlock process
		int** fd = (int**)malloc(sizeof(int) * (n + 1));
		for (int i = 0; i <= n; i++) {
			fd[i] = (int*)malloc(sizeof(int) * 2);
		}

		// Create n child processes
		int process_id = n;
		int tpid = 1;
		for (int i = 0; i < n; i++) {
			if (tpid) {
				tpid = fork();
				if (!tpid) {
					process_id = i;
				}
			}
		}
		printf("process id: %d\n", process_id);

		/*
		*	The commanding process (or deadlock process) computes if the system is in a safe state, and if it is, it finds out which process to
		*	give priority to and sends a signal to the child process (through semaphore operations) to proceed with computation along with information
		*	contained in a pipe. The child processes reads the info that the parent process sends through the pipe, and proceeds until the end is reached
		*	or a "request" or "release" function is called. If the child process sends a request but the request puts the system in a safe state, then the parent
		*	process will not allow for the child process to proceed until it is free.
		*
		*	Since this fork uses the EDF scheduler with LJF as tie-breaker, the commanding process needs to send info about the time when it pipes information.
		*/
		int time = 0;
		int instr_time;
		if (process_id == n) {
			// loop until all processes have finished
			int all_finished = FALSE;
			
			while (!all_finished) {
				// first find the process with the earliest deadline that is not finished and can be executed without putting the system in an unsafe state
				// set all_finished to be true; if there is a process that has not finished execution, set it to false
				
				int idx = 0;
				all_finished = TRUE;
				for (int i = 0; i < n; i++) {
					if (!finish[i]) {
						if (processes[i].deadline < processes[idx].deadline) {
							idx = i;
						} else if ((processes[i].deadline == processes[idx].deadline) 
									&& (processes[i].computation_time > processes[idx].computation_time)) {
							idx == i;
						}
						all_finished = FALSE;
					}
				}

				if (!all_finished) {
					// if the code has not finished yet, then continue execution

					// signal chosen semaphore operation to execute code
					op.sem_num = idx;
					op.sem_op = 1;
					op.sem_flg = 0;
					semop(sid3, &op, 1);

					// wait for semaphore to send information back
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					semop(sid3, &op, 1);

					// the child process will send back a request, a release, or an end
					// read from the pipe dedicated for command process
					read((fd[process_id])[0], line, len);

					printf("From child process: %s", line);

					// read the first integer from line (code for method)
					int code;
					char* tok = strtok(line, " \n");
					tok = strtok(NULL, " ");
					sscanf(tok, "%d", &code);

					switch (code) {
						case 2:
							// if code is 2, the child process is making a request for resources

							// the string returned is of the form: code resrc_#1 resrc_#2 ... resrc_#m
							for (int i = 0; i < m; i++) {
								tok = strtok(NULL, " ");
								sscanf(tok, "%d", &request[i]);

								// subtract the request from available and add to allocation
								available[i] = available[i] - request[i];
								(allocation[idx])[i] = (allocation[idx])[i] + request[i];
							}

							// if this puts the system into an unsafe state, then revert the changes
							// and wait for the resources to become available
							if (!safe_state(n, m, available, max, allocation, finish)) {
								for (int i = 0; i < m; i++) {
									available[i] = available[i] + request[i];
									(allocation[idx])[i] = (allocation[idx])[i] - request[i];
								}

								processes[idx].state = WAITING;
							} else {
								// once the request is satisfied, the process must wait (as per assignment specifications) until
								// another process makes a request or finishes running. if no other processes makes a request, then continue
								processes[idx].state = REQUESTED;
							}
							break;

						case 4:
							// if code is 4, the code is releasing resources

							// the child process returns a string of all the resources it has "popped"
							// the string format is:	code resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... 
							for (int i = 0; i < m; i++) {
								// read the resource index and number of resources released
								int resrc_idx, n_of_resrc;
								tok = strtok(NULL, " ");
								sscanf(tok, "%d", &resrc_idx);
								tok = strtok(NULL, " ");
								sscanf(tok, "%d", &n_of_resrc);

								// get the names of all the resources and push them back onto the stack
								for (int j = 0; j < n_of_resrc; j++) {
									tok = strtok(NULL, " ");
									push_resource(resources, tok, resrc_idx);
								}
							}
						case 5:
							// if code is 5, print the resources used so far
							// the string from the pipe is simply the code
							printf("%d -- Master string: %s", idx + 1, print_master_string(&processes[idx]));
							break;
						default:
							fprintf(stderr, "how did we get here?\nThe string in the pipe: %s\n", line);
							break;
					}
				} else {
					// otherwise, the process has finished
				}
			}
		} else {
			// in child process, wait until deadlock process allows to proceed
			op.sem_num = process_id;
			op.sem_op = -1;
			op.sem_flg = 0;
			semop(sid3, &op, 1);

			// create a buffer to hold numbers
			char dig_buf[5] = "     ";

			struct process curr_proc = processes[process_id];
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
						// put the code to send to the commanding process at the front
						// to indicate what type of request is to be serviced
						char* str_to_send = strdup("2 ");

						// create buffer of size 5
						// clear the buffer
						for (int i = 0; i < 5; i++)
							dig_buf[i] = ' ';

						// load the request array into the string
						char* tok = strtok(string, "(,);\n");
						tok = strtok(NULL, "(,);\n");
						int idx = 0;
						while (tok != NULL) {
							sscanf(tok, "%s", dig_buf);
							sscanf(dig_buf, "%d", &request[idx++]);
							tok = strtok(NULL, "(,);\n");
							strcat(str_to_send, dig_buf);
							
							// clear the buffer
							for (int i = 0; i < 5; i++)
								dig_buf[i] = ' ';
						}
						printf("This is request: %s\n", string);
						printf("This is request string: %s\n", str_to_send);

						// other processes should not be vying for the commanding process resources
						// since no processes should be running concurrently in this assignment
						// therefore, there should not be a need to close the pipes before writing

						// send the string to the commanding process
						write((fd[0])[1], str_to_send, strlen(str_to_send));

						// signal the commanding process to read
						op.sem_num = n;
						op.sem_op = 1;
						op.sem_flg = 0;
						semop(sid3, &op, 1);

						// wait for commanding process to signal to proceed
						op.sem_num = process_id;
						op.sem_op = 1;
						op.sem_flg = 0;
						semop(sid3, &op, 1);

						// once the commanding process has allowed for the process to proceed
						// read what the commanding process has sent through the pipe
						read((fd[process_id])[0], line, len);

						// the string format is: resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... 
						int resrc_idx, n_of_names_resrc;
						char* tok = strtok(string, "(,);\n");
						char* req_word[20];
						for (int i = 0; i < m; i++) {
							sscanf(tok, "%d", &resrc_idx);
							tok = strtok(NULL, "(,);\n");
							sscanf(tok, "%d", &n_of_names_resrc);
							tok = strtok(NULL, "(,);\n");
							for (int j = 0; j < n_of_names_resrc; j++) {
								sscanf(tok, "%s", req_word);
								push_resource(processes[process_id].arr_allocated_resrcs, req_word, resrc_idx);
								tok = strtok(NULL, "(,);\n");
							}
						}

						break;

					// 3 is use_resource
					case 3:
						int y;
						sscanf(string, "use_resources(%d,%d)", &instr_time, &y);
						for (int i = 0; i < m; i++) {
							for (int j = 0; j < y; j++) {
								strcat(processes[process_id].master_str, pop_resource(processes[process_id].arr_allocated_resrcs, i));
							}
						}
						time += instr_time;
						break;

					// 4 is release
					case 4:
						char* str_to_send = strdup("4 ");

						// clear the buffer of size 5
						for (int i = 0; i < 5; i++)
							dig_buf[i] = ' ';

						// load the release request arguments into the string
						char* tok = strtok(string, "(,);\n");
						tok = strtok(NULL, "(,);\n");
						int idx = 0;
						while (tok != NULL) {
							sscanf(tok, "%s", dig_buf);
							tok = strtok(NULL, "(,);\n");
							strcat(str_to_send, dig_buf);
							
							// clear the buffer
							for (int i = 0; i < 5; i++)
								dig_buf[i] = ' ';
						}

						// send the string to the commanding process
						write((fd[0])[1], str_to_send, strlen(str_to_send));

						// signal the commanding process to read
						op.sem_num = n;
						op.sem_op = 1;
						op.sem_flg = 0;
						semop(sid3, &op, 1);

						// wait for commanding process to signal to proceed
						op.sem_num = process_id;
						op.sem_op = 1;
						op.sem_flg = 0;
						semop(sid3, &op, 1);

						break;

					// 5 is print_resources_used
					// print process number and master string
					case 5:
						printf("Print resource\n");
						break;

					// 6 is end
					case 6:
						printf("End process\n");
						break;
					
					default:
						fprintf(stderr, "Wrong n returned\n");
						exit(1);
				}

				string = deque_instruction(&curr_proc.proc_instructions);
			}
		}

		fclose(output);

		// destroy the semaphores for the processes
		semctl(sid3, 0, IPC_RMID, 0);

		// free the file descriptors
		for (int i = 0; i <= n; i++) {
			free(fd[i]);
		}
		free(fd);

		op.sem_num = m;
		op.sem_op = 1;
		op.sem_flg = 0;
		semop(sid2, &op, 1);

	} else {
		// execute EDL and LLF seperatly
		op.sem_num = m;	// the last semaphore
		op.sem_op = -1;
		op.sem_flg = 0;
		semop(sid2, &op, 1);

		FILE* output = fopen("temp2.txt", "w");
		printf("The bottom code executed\n");

		// check if the system is initially in a safe state
		int safe = safe_state(n, m, available, max, allocation, finish);

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

