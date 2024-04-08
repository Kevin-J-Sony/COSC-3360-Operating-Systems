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

#include "simple_process.h"

#define TRUE (1 == 1)
#define FALSE (1 == 0)

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

/*
*	Number to string function. Takes in number, returns a string
*/
char* numb_to_string(int numb) {
	char dig_buf[5];
	sprintf(dig_buf, "%d", numb);
	return strdup(dig_buf);
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
	//printf("inside print master string\n");
	// we first get the number of strings outputted
	int size_str_arr = p_process->n_str_master;
	//printf("%d\n", size_str_arr);
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
	char* final_string = strdup(" ");
	for (int i = 0; i < size_str_arr; i++) {
		strcat(final_string, total_str[i]);
		//printf("%s\n", total_str[i]);
	}
	//printf("exiting print master string\n");

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
	*	FINISH is an array that indicates whether process at an index is finished
	*/
	int* available = (int*)malloc(sizeof(int) * m);
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
		processes[i].master_str = strdup("");
		processes[i].state = CREATED;
		processes[i].deadline_misses = 0;

		processes[i].arr_allocated_resrcs = (struct resource**)malloc(sizeof(struct resource*) * m);
		processes[i].request_array = (int*)malloc(sizeof(int) * m);
		for (int j = 0; j < m; j++) {
			processes[i].arr_allocated_resrcs[j] = (struct resource*) NULL;;
			processes[i].request_array[j] = 0;
		}
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
	key_t key1 = ftok("temp1.txt", 'A');
	int sid1 = semget(key1, 1, 0666 | IPC_CREAT);
	
	// create n + 1 semaphores, one for each child process, and one for deadlock process
	// parent process has process_id = n, so the semaphore corresponding to it is semaphore @ n
	key_t key2 = ftok("temp2.txt", 'B');
	int sid3 = semget(key2, n + 1, 0666 | IPC_CREAT);

	if (sid1 == -1) {
        fprintf(stderr, "semget failed: %s\n", strerror(errno));
	}
	

	/*
	*	In decrementing a semaphore, the following instructions are used
	*		op.sem_num = idx;
	*		op.sem_op = -1;
	*		op.sem_flg = 0;
	*		semop(sid, &op, 1);
	*	In incrementing a semaphore, the following instructions are executed
	*		op.sem_num = idx;
	*		op.sem_op = -1;
	*		op.sem_flg = 0;
	*		semop(sid, &op, 1);
	*/
	/*
	op.sem_num = 0;
	op.sem_op = 1;
	op.sem_flg = 0;
	int t = semop(sid1, &op, 1);
	*/

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
				}
				// store the instructions in the vector					
				enqueue_instruction(&processes[proc_count].proc_instructions, line);				
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
	memset(line, 0, sizeof(line));

	// test if resource stack works
	/*
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
	}*/
	
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
	int error;
	if (split) {

		sleep(10);

		// execute EDL and LLF seperatly
		
		op.sem_num = 0;
		op.sem_op = -1;
		op.sem_flg = 0;
		error = semop(sid1, &op, 1);
		if (error == -1) {
			fprintf(stderr, "\nsemop error(1) is: %s\n", strerror(errno));
		}
		
		printf("=======================================\nBANKER'S ALGORITHM WITH EDF SCHEDULING\n=======================================\n");


		// check if the system is initially in a safe state
		int safe = safe_state(n, m, available, max, allocation, finish);

		// create n + 1 file descriptors, one for each child process, and one for deadlock process
		int** fd = (int**)malloc(sizeof(int) * (n + 1));
		for (int i = 0; i <= n; i++) {
			fd[i] = (int*)malloc(sizeof(int) * 2);
			pipe(fd[i]);
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

		/*
		*	The commanding process (or deadlock process) computes if the system is in a safe state, and if it is, it finds out which process to
		*	give priority to and sends a signal to the child process (through semaphore operations) to proceed with computation along with information
		*	contained in a pipe. The child processes reads the info that the parent process sends through the pipe, and proceeds until the end is reached
		*	or a "request" or "release" function is called. If the child process sends a request but the request puts the system in a safe state, then the parent
		*	process will not allow for the child process to proceed until it is free.
		*
		*	Since this fork uses the EDF scheduler with LJF as tie-breaker, the commanding process needs to send info about the time when it pipes information.
		*/
		int time;
		int instr_time;
		if (process_id == n) {
			// loop until all processes have finished
			// the time is given by the commanding (deadlock) process
			time = 0;
			int all_finished = FALSE;
			
			while (!all_finished) {
				// first check if there is any waiting state that can proceed if request can be satisfied
				int idx = 0;
				for (int i = 0; i < n; i++) {
					if (processes[i].state == WAITING) {
						printf("process %d is wating\n", i);
						safe = TRUE;
						for (int j = 0; j < m; j++) {
							if (available[i] < processes[i].request_array[j]) {
								safe = FALSE;
							}
							available[i] = available[i] - processes[i].request_array[j];
						}
						safe = safe && safe_state(n, m, available, max, allocation, finish);

						if (safe) {
							processes[i].state = READY;
							printf("process %d is ready\n", i);
							idx = i;
						}
						for (int j = 0; j < m; j++) {
							available[i] = available[i] + processes[i].request_array[j];
						}
					} else {
						idx = i;
					}
				}

				// then find the process with the earliest deadline that is not finished and can be executed without putting the system in an unsafe state
				// set all_finished to be true; if there is a process that has not finished execution, set it to false

				all_finished = TRUE;
				for (int i = 0; i < n; i++) {
					if (!finish[i]) {
						//printf("process %d state is: %d\n", i, processes[i].state);
						if (processes[i].state != WAITING && processes[i].state != REQUESTED) {
							if (processes[idx].state == WAITING || processes[idx].state == REQUESTED) {
								idx = i;
							} else if (processes[i].deadline < processes[idx].deadline) {
								idx = i;
							} else if ((processes[i].deadline == processes[idx].deadline) 
										&& (processes[i].computation_time > processes[idx].computation_time)) {
								idx = i;
							}
							all_finished = FALSE;
						}
					}
				}

				// next, change all processes with a state of REQUESTED to a state of READY
				for (int i = 0; i < n; i++) {
					if (!finish[i]) {
						if (processes[idx].state == REQUESTED) {
							processes[idx].state = READY;
						}
					}
				}

				// if all_finished is TRUE, rerun the banker's algorithm to check if REQUESTED wasn't only one remaining
				if (all_finished) {
					for (int i = 0; i < n; i++) {
						if (!finish[i] && (processes[i].state != WAITING)) {
							if (processes[i].deadline < processes[idx].deadline) {
								idx = i;
							} else if ((processes[i].deadline == processes[idx].deadline) 
										&& (processes[i].computation_time > processes[idx].computation_time)) {
								idx == i;
							}
							all_finished = FALSE;
						}
					}
				}
				printf("valid idx = %d\n", idx);

				if (!all_finished) {
					// if the code has not finished yet, then continue execution
					// write to the process
					if (processes[idx].state == CREATED) {
						write((fd[idx])[1], numb_to_string(time), strlen(numb_to_string(time)));
						write((fd[idx])[1], "  ", strlen("  "));
						printf("deadlock time: %s\n", numb_to_string(time));
					}
					processes[idx].state = RUNNING;

					// signal chosen semaphore operation to execute code
					op.sem_num = idx;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(2) is: %s\n", strerror(errno));
					}

					// process executes a bunch of instructions, so wait for it to signal when it requests for resources, releases resources
					// sends message to print, or ends execution
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(3) is: %s\n", strerror(errno));
					}

					// the child process will send back a request, a release, or an end
					// read from the pipe dedicated for command process
					error = read((fd[process_id])[0], line, len);
					if (error == -1) {
						fprintf(stderr, "\nread error is: %s\n", strerror(errno));
					}

					printf("From child process(%d): %s\n", idx, line);

					// read the first integer from line (code for method)
					int code;
					char* tok = strtok(strdup(line), " \n");

					sscanf(tok, "%d", &code);

					// after reading the message from the child process and determining what to do
					// send a status code that the child process can read to know what the parent process
					// has done with the message sent
					int status_code;
					char* str_to_send = strdup("");

					if (code == 2) {
						// if code is 2, the child process is making a request for resources

						// the string returned is of the form: code resrc_#1 resrc_#2 ... resrc_#m time
						int negative_resrcs = FALSE;
						for (int i = 0; i < m; i++) {
							tok = strtok(NULL, " ");
							sscanf(tok, "%d", &processes[idx].request_array[i]);

							// subtract the request from available and add to allocation
							available[i] = available[i] - processes[idx].request_array[i];
							if (available[i] < 0)
								negative_resrcs = TRUE;
							(allocation[idx])[i] = (allocation[idx])[i] + processes[idx].request_array[i];
						}

						// read the time
						tok = strtok(NULL, " ");
						sscanf(tok, "%d", &time);

						// if this puts the system into an unsafe state, then revert the changes
						// and wait for the resources to become available
						if (negative_resrcs || !safe_state(n, m, available, max, allocation, finish)) {
							for (int i = 0; i < m; i++) {
								available[i] = available[i] + processes[idx].request_array[i];
								(allocation[idx])[i] = (allocation[idx])[i] - processes[idx].request_array[i];
							}

							processes[idx].state = WAITING;

							// sent this status code to the child process to indicate that the process will need to wait
							// and resubmit the string again at a later time
							status_code = 1;
							strcat(str_to_send, strdup("1 "));
						} else {
							// once the request is satisfied, the process must wait (as per assignment specifications) until
							// another process makes a request or finishes running. if no other processes makes a request, then continue
							processes[idx].state = REQUESTED;

							// send this status code to the child process to tell that it to read the pipe to get the resources it wants
							status_code = 0;
							strcat(str_to_send, strdup("0 "));

							// then send the request names in the format: status_code resrc_idx1 #_req_resrc_1 resrc_1_name_1 ... #_req_resrc_2 resrc_2_name_1 ... ... #_req_resrc_m resrc_m_name_1 ... 
							for (int i = 0; i < m; i++) {
								strcat(str_to_send, numb_to_string(i));
								strcat(str_to_send, strdup(" "));
								strcat(str_to_send, numb_to_string(processes[idx].request_array[i]));
								strcat(str_to_send, strdup(" "));
								
								for (int j = 0; j < processes[idx].request_array[i]; j++) {
									strcat(str_to_send, pop_resource(resources, i));
									strcat(str_to_send, strdup(" "));
								}
							}
							//printf("string: %s\n", str_to_send);
							//printf("request value: ");
						}

						printf("\n\n\nprocess(%d) status(%d)\n", idx, status_code);

						
						// print out the state of the system
						// print the available resources
						for (int i = 0; i < m; i++) {
							printf("available[%d] = %d\n", i, available[i]);
						}

						// print the resources currently allocated to the process
						printf("allocation = ");
						for (int i = 0; i < m; i++) {
							printf("%d ", (allocation[idx])[i]);
						}
						printf("\n");

						// print the remaining resources needed by the process
						printf("need = ");
						for (int i = 0; i < m; i++) {
							printf("%d ", (max[idx])[i] - (allocation[idx])[i]);
						}
						printf("\n");

						// write response to child process
						write((fd[idx])[1], str_to_send, strlen(str_to_send));

						// signal the requesting process semaphore operation to execute code
						op.sem_num = idx;
						op.sem_op = 1;
						op.sem_flg = 0;
						error = semop(sid3, &op, 1);
						if (error == -1) {
							fprintf(stderr, "\nsemop error(4) is: %s\n", strerror(errno));
						}

						// wait for the requesting process to interpret the message sent
						op.sem_num = process_id;
						op.sem_op = -1;
						op.sem_flg = 0;
						error = semop(sid3, &op, 1);
						if (error == -1) {
							fprintf(stderr, "\nsemop error(5) is: %s\n", strerror(errno));
						}
						
					} else if (code == 4) {
						// if code is 4, the code is releasing resources
						printf("wow\n");

						// the child process returns a string of all the resources it has "popped"
						// the string format is:	code resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... time
						for (int i = 0; i < m; i++) {
							// read the resource index and number of resources released
							int resrc_idx, n_of_resrc;
							tok = strtok(NULL, " ");
							sscanf(tok, "%d", &resrc_idx);
							tok = strtok(NULL, " ");
							sscanf(tok, "%d", &n_of_resrc);
							available[i] = available[i] + n_of_resrc;
							(allocation[idx])[i] = (allocation[idx])[i] - n_of_resrc;
							printf("n_of_resrc = %d, allocation[%d][%d] = %d, available[%d] = %d\n", n_of_resrc, idx, i, (allocation[idx])[i], i, available[i]);

							// get the names of all the resources and push them back onto the stack
							for (int j = 0; j < n_of_resrc; j++) {
								tok = strtok(NULL, " ");
								push_resource(resources, tok, resrc_idx);
							}
						}

						// there is no need to signal the code since the released resources might allow for previously waiting
						// processes to continue

					} else if (code == 5) {
						// if code is 5, print the resources used so far
						// the string from the pipe is simply the code, the deadline misses, and the remaining is the master string
						
						// print the available resources
						for (int i = 0; i < m; i++) {
							printf("available[%d] = %d\n", i, available[i]);
						}

						// print the resources currently allocated to the process
						printf("allocation = ");
						for (int i = 0; i < m; i++) {
							printf("%d ", (allocation[idx])[i]);
						}
						printf("\n");

						// print the remaining resources needed by the process
						printf("need = ");
						for (int i = 0; i < m; i++) {
							printf("%d ", (max[idx])[i] - (allocation[idx])[i]);
						}
						printf("\n");

						
						tok = strtok(NULL, " ");
						sscanf(tok, "%d", &processes[idx].deadline_misses);
						printf("deadline misses = %d\n", processes[idx].deadline_misses);
						
						//printf("%d -- Master string: %s", idx + 1, print_master_string(&processes[idx]));
						printf("%d -- Master string: ", idx + 1);
						while (tok != NULL) {
							tok = strtok(NULL, " ");
							printf("%s ", tok);
						}

						// signal to child process that deadlock process has recieved print instructions
						op.sem_num = idx;
						op.sem_op = 1;
						op.sem_flg = 0;
						error = semop(sid3, &op, 1);
						if (error == -1) {
							fprintf(stderr, "\nsemop error(8) is: %s\n", strerror(errno));
						}
						

					} else if (code == 6) {
						// if code is 6, then the process has finished execution
						
						// it functions like a release
						// the child process returns a string of all the resources it has "popped"
						// the string format is:	code resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... time
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

						finish[idx] = TRUE;
					} else {
						fprintf(stderr, "how did we get here?\nThe string in the pipe: %s\n", line);
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
			error = semop(sid3, &op, 1);
			if (error == -1) {
				fprintf(stderr, "\nsemop error(9) is: %s\n", strerror(errno));
			}

			if (process_id == 1) {
				printf("finally here\n");
			}

			// read the current time from the pipe
			read((fd[process_id])[0], line, len);
			sscanf(line, "%d", &time);

			char* string = deque_instruction(&processes[process_id].proc_instructions);
			while (string != NULL) {
				int n = parse_instruction(string);

				// check if any deadline misses occurred
				if (time > processes[process_id].deadline) {
					processes[process_id].deadline_misses++;
				}

				// storing all previous declarations so that the gcc doesn't throw a fit
				int n_of_names_resrc = -1;
				char* tok;
				char* str_to_send;
				int idx;
				int status_code;

				/*
				if (process_id == 0) {
					printf("\ntime: %d, next instruction: %s", time, string);
					printf("line: %s\n", line);
				}
				*/

				if (n == 1) {
					// 1 is calculate
					sscanf(string, "calculate(%d)", &instr_time);
					time += instr_time;
				} else if (n == 2) {
					// 2 is request
					// the assignment demands that after each request, print the state of the process
					
					// the request instruction takes 1 computation time
					time += 1;

					// put the code to send to the commanding process at the front
					// to indicate what type of request is to be serviced
					str_to_send = strdup("2 ");

					// load the request array into the string
					tok = strtok(strdup(string), "(,);\n");
					tok = strtok(NULL, "(,);\n");
					idx = 0;
					while (tok != NULL) {
						sscanf(tok, "%d", &processes[process_id].request_array[idx]);
						tok = strtok(NULL, "(,);\n");
						strcat(str_to_send, numb_to_string(processes[process_id].request_array[idx++]));
					}
					// append time to the end of the string
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(time));

					//printf("string to send: %s\n", str_to_send);

					//printf("This is request: %s\n", string);
					//printf("This is request string: %s\n", str_to_send);

					// other processes should not be vying for the commanding process resources
					// since no processes should be running concurrently in this assignment
					// therefore, there should not be a need to close the pipes before writing

					// send the string to the commanding process
					write((fd[n])[1], str_to_send, strlen(str_to_send));

					// signal the commanding process to read
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(10) is: %s\n", strerror(errno));
					}
					
					// wait for commanding process to send the status code
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(11) is: %s\n", strerror(errno));
					}

					// once the commanding process has allowed for the process to proceed
					// read what the commanding process has sent through the pipe
					read((fd[process_id])[0], line, len);

					printf("line: %s\n", line);

					// get the status code
					tok = strtok(line, " (,);\n");
					sscanf(tok, "%d", &status_code);

					
					if (status_code == 0) {
						// the string format is: resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... 
						int resrc_idx, n_of_names_resrc;

						char req_word[20];
						for (int i = 0; i < m; i++) {
							tok = strtok(NULL, " (,);\n");
							sscanf(tok, "%d", &resrc_idx);

							tok = strtok(NULL, " (,);\n");
							sscanf(tok, "%d", &n_of_names_resrc);

							for (int j = 0; j < n_of_names_resrc; j++) {
								tok = strtok(NULL, " (,);\n");
								sscanf(tok, "%s", req_word);
								push_resource(processes[process_id].arr_allocated_resrcs, req_word, resrc_idx);
							}
						}

					} else if (status_code == 1) {
						// store the previous string instruction
						struct instruction* temp = processes[process_id].proc_instructions;
						struct instruction* new_head = (struct instruction*)malloc(sizeof(struct instruction));
						new_head->instruction = strdup(string);
						new_head->next_instruction = temp;
						processes[process_id].proc_instructions = new_head;
					} else {
						printf("WHAT HAS HAPPENED HERE\n");
					}

					printf("\n%d -- Master string: %s\n\n\n", process_id + 1, print_master_string(&processes[process_id]));


					// signal to deadlock process that child has read response
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(12) is: %s\n", strerror(errno));
					}

					// finally, wait for the deadlock process to signal to proceed
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(13) is: %s\n", strerror(errno));
					}
					printf("finally free %d\n", process_id);

					if (status_code == 1) {
						printf("since status code is 1, string should be: %s", processes[process_id].proc_instructions->instruction);
					}


				} else if (n == 3) {
					// 3 is use_resource
					int y;
					sscanf(string, "use_resources(%d,%d)", &instr_time, &y);
					struct resources* temp = NULL;
					char* t_str;
					for (int i = 0; i < m; i++) {
						struct resource* temp = (processes[process_id].arr_allocated_resrcs[i]);
						for (int j = 0; j < y && temp != NULL; j++) {
							t_str = temp->resource_name;
							strcat(processes[process_id].master_str, strdup(t_str));
							temp = temp->next_resource;
							processes[process_id].n_str_master++;
						}
					}
					time += instr_time;

				} else if (n == 4) {
					// 4 is release
					// send code and update time
					time += 1;
					str_to_send = strdup("4 ");


					// interpret the release instruction arguments into the string
					// the string format is: code resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... time

					tok = strtok(string, "(,);\n");
					tok = strtok(NULL, "(,);\n");
					idx = 0;
					while (tok != NULL) {
						sscanf(tok, "%d", &n_of_names_resrc);
						tok = strtok(NULL, "(,);\n");
						strcat(str_to_send, numb_to_string(n_of_names_resrc));
						
						for (int j = 0; j < n_of_names_resrc; j++) {
							strcat(str_to_send, strdup(" "));
							strcat(str_to_send, strdup(pop_resource(processes[process_id].arr_allocated_resrcs, idx)));
						}
						idx++;
					}
					
					// append time to the end of the string
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(time));
					printf("str_to_send: %s\n", str_to_send);

					// send the string to the commanding process
					error = write((fd[n])[1], str_to_send, strlen(str_to_send));
					if (error == -1) {
						fprintf(stderr, "\nwrite(1) is: %s\n", strerror(errno));
					}

					// signal the commanding process to read
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(14) is: %s\n", strerror(errno));
					}

					// wait for commanding process to signal to proceed
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(15) is: %s\n", strerror(errno));
					}
					// once the commanding process returns to it, there's nothing more to execute for this instruction

				} else if (n == 5) {
					// 5 is print_resources_used
					// print process number, the number of deadline misses and master string, and time at the end of the string
					printf("Print resource\n");
					// print_resource_used takes a computation time
					// the string sent is the code(5) and the current time
					time += 1;
					str_to_send = strdup("5 ");

					// put deadline
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(time));

					// append time to the end of the string
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(time));
					
					// send the string to the commanding process
					write((fd[n])[1], str_to_send, strlen(str_to_send));

					// signal the commanding process to read
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(16) is: %s\n", strerror(errno));
					}

					// wait for commanding process to signal to proceed
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(17) is: %s\n", strerror(errno));
					}


				} else if (n == 6) {
					// 6 is end
					// with end process, no computation time is taken, and all resources are released
					str_to_send = strdup("6 ");


					// it is NOT the same thing as print_resource_used, but instead, more like a release
					// the string format is: code resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... time

					for (int i = 0; i < m; i++) {
						// append the resource_type to the string
						strcat(str_to_send, numb_to_string(i));
						strcat(str_to_send, strdup(" "));
						
						int n_of_resrc;
						char* str = pop_resource(processes[process_id].arr_allocated_resrcs, i);
						char* m_str = strdup(" ");
						while (str != NULL) {
							strcat(m_str, str);
							strcat(m_str, strdup(" "));
							str = pop_resource(processes[process_id].arr_allocated_resrcs, i);
						}
						strcat(str_to_send, m_str);
					}
					/*
					if (process_id == 0) {
						printf("string to send is: %s\n", str_to_send);
					}*/

					// send the string to the commanding process
					write((fd[n])[1], str_to_send, strlen(str_to_send));

					// signal the commanding process to read
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(18) is: %s\n", strerror(errno));
					}
					printf("End process\n");

					exit(EXIT_SUCCESS);
				} else {
					fprintf(stderr, "Invalid instruction\n");
					// proceed like a normal exit
					// with end process, no computation time is taken, and all resources are released
					str_to_send = strdup("6 ");


					// it is NOT the same thing as print_resource_used, but instead, more like a release
					// the string format is: code resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... time

					for (int i = 0; i < m; i++) {
						// append the resource_type to the string
						strcat(str_to_send, numb_to_string(i));
						strcat(str_to_send, strdup(" "));
						
						int n_of_resrc;
						char* str = pop_resource(processes[process_id].arr_allocated_resrcs, i);
						char* m_str = strdup(" ");
						while (str != NULL) {
							strcat(m_str, str);
							strcat(m_str, strdup(" "));
							str = pop_resource(processes[process_id].arr_allocated_resrcs, i);
						}
						strcat(str_to_send, m_str);
					}
					/*
					if (process_id == 0) {
						printf("string to send is: %s\n", str_to_send);
					}*/

					// send the string to the commanding process
					write((fd[n])[1], str_to_send, strlen(str_to_send));

					// signal the commanding process to read
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(18) is: %s\n", strerror(errno));
					}
					printf("End process\n");

					exit(EXIT_FAILURE);
				}

				string = deque_instruction(&processes[process_id].proc_instructions);
			}

			// process has finished executing all the strings
			fprintf(stderr, "The process should have not gotten here\n");
			exit(EXIT_FAILURE);
		}


		// free the file descriptors
		for (int i = 0; i <= n; i++) {
			free(fd[i]);
		}
		free(fd);

		/*
		op.sem_num = 0;
		op.sem_op = 1;
		op.sem_flg = 0;
		error = semop(sid1, &op, 1);
		if (error == -1) {
			fprintf(stderr, "\nsemop error(19) is: %s\n", strerror(errno));
		}
		*/

	} else {

		// execute EDL and LLF seperatly
		/*
		op.sem_num = 0;	// the last semaphore
		op.sem_op = -1;
		op.sem_flg = 0;
		int error = semop(sid1, &op, 1);
		if (error == -1) {
			fprintf(stderr, "\nsemop error(20) is: %s\n", strerror(errno));
		}*/

		printf("=======================================\nBANKER'S ALGORITHM WITH LLF SCHEDULING\n=======================================\n");
		/*
		*	The key difference which distinguishes the LLF scheduler from the EDL scheduler is that after each instruction, the deadlock process
		*	must check if a context switch must occur.
		*/






















		// create n + 1 file descriptors, one for each child process, and one for deadlock process
		int** fd = (int**)malloc(sizeof(int) * (n + 1));
		for (int i = 0; i <= n; i++) {
			fd[i] = (int*)malloc(sizeof(int) * 2);
			pipe(fd[i]);
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
		printf("process_id: %d\n", process_id);

		/*
		*	The commanding process (or deadlock process) computes if the system is in a safe state, and if it is, it finds out which process to
		*	give priority to and sends a signal to the child process (through semaphore operations) to proceed with computation along with information
		*	contained in a pipe. The child processes reads the info that the parent process sends through the pipe, and proceeds until the end is reached
		*	or a "request" or "release" function is called. If the child process sends a request but the request puts the system in a safe state, then the parent
		*	process will not allow for the child process to proceed until it is free.
		*
		*	Since this fork uses the LLF scheduler with SJF as tie-breaker, the commanding process needs to send info about the time when it pipes information.
		*/
		// get the values of the semaphore
		int time;
		int instr_time;
		int safe;
		if (process_id == n) {
			// loop until all processes have finished
			// the time is given by the commanding (deadlock) process
			time = 0;
			int all_finished = FALSE;
			
			while (!all_finished) {
				// first check if there is any waiting state that can proceed if request can be satisfied
				int idx = 0;
				for (int i = 0; i < n; i++) {
					if (processes[i].state == WAITING) {
						safe = TRUE;
						for (int j = 0; j < m; j++) {
							if (available[i] < processes[i].request_array[j]) {
								safe = FALSE;
							}
							available[i] = available[i] - processes[i].request_array[j];
						}
						safe = safe && safe_state(n, m, available, max, allocation, finish);

						if (safe) {
							processes[i].state = READY;
							printf("process %d is ready\n", i);
							idx = i;
						}
						for (int j = 0; j < m; j++) {
							available[i] = available[i] + processes[i].request_array[j];
						}
					} else {
						idx = i;
					}
				}

				// then find the process with the lowest laxity that is not finished and can be executed without putting the system in an unsafe state
				// set all_finished to be true; if there is a process that has not finished execution, set it to false
				int low_lax = processes[idx].deadline - time - processes[idx].remaining_computation_time;
				all_finished = TRUE;
				for (int i = 0; i < n; i++) {
					if (!finish[i]) {
						printf("process %d state is: %d\n", i, processes[i].state);
						if (processes[i].state != WAITING && processes[i].state != REQUESTED) {
							int curr_lax = processes[i].deadline - time - processes[i].remaining_computation_time;
							if (curr_lax < low_lax) {
								idx = i;
								curr_lax = low_lax;
							} else if ((curr_lax == low_lax) &&  (processes[i].computation_time < processes[idx].computation_time)) {
								idx = i;
								curr_lax = low_lax;
							}
							all_finished = FALSE;
						}
					}
				}

				// next, change all processes with a state of REQUESTED to a state of READY
				// if all_finished is TRUE, then rereun the banker's algorithm so that
				for (int i = 0; i < n; i++) {
					if (!finish[i]) {
						if (processes[i].state == REQUESTED) {
							processes[i].state = READY;
							if (all_finished) {
								int curr_lax = processes[i].deadline - time - processes[i].remaining_computation_time;
								if (curr_lax < low_lax) {
									idx = i;
									curr_lax = low_lax;
								} else if ((curr_lax == low_lax) &&  (processes[i].computation_time < processes[idx].computation_time)) {
									idx = i;
									curr_lax = low_lax;
								}
							}
						}
					}
				}

				printf("valid idx = %d\n", idx);

				if (!all_finished) {
					// if the code has not finished yet, then continue execution
					// write to the process
					if (processes[idx].state == CREATED) {
						write((fd[idx])[1], numb_to_string(time), strlen(numb_to_string(time)));
						write((fd[idx])[1], "  ", strlen("  "));
						printf("deadlock time: %s\n", numb_to_string(time));
					}
					processes[idx].state = RUNNING;

					// signal chosen semaphore operation to execute code
					op.sem_num = idx;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(21) is: %s\n", strerror(errno));
					}

					// child process will execute an instruction, so wait for it to signal
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(22) is: %s\n", strerror(errno));
					}
				}

				// once the child process has finished executing an instruction
				// it will send a message along the pipe

				// the child process will send back a request, a release, or an end
				// read from the pipe dedicated for command process
				error = read((fd[process_id])[0], line, len);
				if (error == -1) {
					fprintf(stderr, "\nread error is: %s\n", strerror(errno));
				}

				printf("From child process(%d): %s\n", idx, line);

				// the first integer in the line gives the code
				int code;
				char* tok = strtok(strdup(line), " \n");
				sscanf(tok, "%d", &code);
				printf("code %d has been read: %s\n", code, line);


				// after getting the code of the message, we can determine the instruction the code ran
				if (code == 1) {
					// code returned is 1
					// the instruction executed is calculate
					// the format of the string is: code time remaining_comp_time
					tok = strtok(NULL, " \n");
					sscanf(tok, "%d", &time);
					tok = strtok(NULL, " \n");
					sscanf(tok, "%d", &processes[idx].remaining_computation_time);
				} else if (code == 2) {
					// the child process is requesting resources
					// the format of the string is: code resrc_#1 resrc_#2 ... resrc_#m time remaining_comp_time

					// make sure that the process isn't requesting more resources than available
					int negative_resrcs = FALSE;
					for (int i = 0; i < m; i++) {
						tok = strtok(NULL, " ");
						sscanf(tok, "%d", &processes[idx].request_array[i]);

						// subtract the request from available and add to allocation
						available[i] = available[i] - processes[idx].request_array[i];
						if (available[i] < 0)
							negative_resrcs = TRUE;
						(allocation[idx])[i] = (allocation[idx])[i] + processes[idx].request_array[i];
					}

					// read the time and the remaining computation time of the process
					tok = strtok(NULL, " ");
					sscanf(tok, "%d", &time);
					tok = strtok(NULL, " ");
					sscanf(tok, "%d", &processes[idx].remaining_computation_time);
					

					// if this puts the system into an unsafe state, then revert the changes
					// and wait for the resources to become available
					int status_code;
					char* str_to_send = strdup("");
					if (negative_resrcs || !safe_state(n, m, available, max, allocation, finish)) {
						for (int i = 0; i < m; i++) {
							available[i] = available[i] + processes[idx].request_array[i];
							(allocation[idx])[i] = (allocation[idx])[i] - processes[idx].request_array[i];
						}

						processes[idx].state = WAITING;

						// sent this status code to the child process to indicate that the process will need to wait
						// and resubmit the string again at a later time
						status_code = 1;
						strcat(str_to_send, strdup("1 "));
					} else {
						// once the request is satisfied, the process must wait (as per assignment specifications) until
						// another process makes a request or finishes running. if no other processes makes a request, then continue
						processes[idx].state = REQUESTED;

						// send this status code to the child process to tell that it to read the pipe to get the resources it wants
						status_code = 0;
						strcat(str_to_send, strdup("0 "));

						// then send the request names in the format: status_code resrc_idx1 #_req_resrc_1 resrc_1_name_1 ... #_req_resrc_2 resrc_2_name_1 ... ... #_req_resrc_m resrc_m_name_1 ... 
						for (int i = 0; i < m; i++) {
							strcat(str_to_send, numb_to_string(i));
							strcat(str_to_send, strdup(" "));
							strcat(str_to_send, numb_to_string(processes[idx].request_array[i]));
							strcat(str_to_send, strdup(" "));
							
							for (int j = 0; j < processes[idx].request_array[i]; j++) {
								strcat(str_to_send, pop_resource(resources, i));
								strcat(str_to_send, strdup(" "));
							}
						}
						//printf("string: %s\n", str_to_send);
						//printf("request value: ");
					}

					printf("\n\n\nprocess(%d) status(%d)\n", idx, status_code);

					
					// print out the state of the system
					// print the available resources
					for (int i = 0; i < m; i++) {
						printf("available[%d] = %d\n", i, available[i]);
					}

					// print the resources currently allocated to the process
					printf("allocation = ");
					for (int i = 0; i < m; i++) {
						printf("%d ", (allocation[idx])[i]);
					}
					printf("\n");

					// print the remaining resources needed by the process
					printf("need = ");
					for (int i = 0; i < m; i++) {
						printf("%d ", (max[idx])[i] - (allocation[idx])[i]);
					}
					printf("\n");

					printf("string to send: %s\n", str_to_send);

					// write response to child process
					write((fd[idx])[1], str_to_send, strlen(str_to_send));

					// signal the requesting process semaphore operation to execute code
					op.sem_num = idx;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(23) is: %s\n", strerror(errno));
					}

					// wait for the requesting process to interpret the message sent
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(24) is: %s\n", strerror(errno));
					}

				} else if (code == 3) {
					// code returned is 3
					// the instruction executed is use_resources
					// the format of the string is: code time remaining_comp_time
					tok = strtok(NULL, " \n");
					sscanf(tok, "%d", &time);
					tok = strtok(NULL, " \n");
					sscanf(tok, "%d", &processes[idx].remaining_computation_time);
				} else if (code == 4) {
					// if code is 4, the code is releasing resources

					// the child process returns a string of all the resources it has "popped"
					// the format of the string is: code resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... time remaining_comp_time
					for (int i = 0; i < m; i++) {
						// read the resource index and number of resources released
						int resrc_idx, n_of_resrc;
						tok = strtok(NULL, " ");
						sscanf(tok, "%d", &resrc_idx);
						tok = strtok(NULL, " ");
						sscanf(tok, "%d", &n_of_resrc);
						available[i] = available[i] + n_of_resrc;
						(allocation[idx])[i] = (allocation[idx])[i] - n_of_resrc;
						printf("n_of_resrc = %d, allocation[%d][%d] = %d, available[%d] = %d\n", n_of_resrc, idx, i, (allocation[idx])[i], i, available[i]);

						// get the names of all the resources and push them back onto the stack
						for (int j = 0; j < n_of_resrc; j++) {
							tok = strtok(NULL, " ");
							push_resource(resources, tok, resrc_idx);
						}
					}

					// get the time and remaining_computation_time
					tok = strtok(NULL, " \n");
					sscanf(tok, "%d", &time);
					tok = strtok(NULL, " \n");
					sscanf(tok, "%d", &processes[idx].remaining_computation_time);

					// there is no need to signal the code since the released resources might allow for previously waiting
					// processes to continue

				} else if (code == 5) {
					// if code is 5, print the resources used so far
					// the format of the string is: code deadline_misses time remaining_comp_time
					
					// print the available resources
					for (int i = 0; i < m; i++) {
						printf("available[%d] = %d\n", i, available[i]);
					}

					// print the resources currently allocated to the process
					printf("allocation = ");
					for (int i = 0; i < m; i++) {
						printf("%d ", (allocation[idx])[i]);
					}
					printf("\n");

					// print the remaining resources needed by the process
					printf("need = ");
					for (int i = 0; i < m; i++) {
						printf("%d ", (max[idx])[i] - (allocation[idx])[i]);
					}
					printf("\n");
					
					// get deadline misses, time, and remaining computation time
					tok = strtok(NULL, " ");
					sscanf(tok, "%d", &processes[idx].deadline_misses);
					tok = strtok(NULL, " ");
					sscanf(tok, "%d", &time);
					tok = strtok(NULL, " ");
					sscanf(tok, "%d", &processes[idx].remaining_computation_time);
					
					printf("deadline misses = %d\n", processes[idx].deadline_misses);


					// signal to child process that deadlock process has recieved print instructions
					op.sem_num = idx;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(25) is: %s\n", strerror(errno));
					}

					// wait for child process to print master string
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(26) is: %s\n", strerror(errno));
					}
				} else if (code == 6) {
					// if code is 6, then the process has finished execution
					// it functions like a release

					// the child process returns a string of all the resources it has "popped"
					// the format of the string is: code resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... time remaining_comp_time
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

					// get the time and remaining_computation_time
					tok = strtok(NULL, " \n");
					sscanf(tok, "%d", &time);
					tok = strtok(NULL, " \n");
					sscanf(tok, "%d", &processes[idx].remaining_computation_time);

					finish[idx] = TRUE;
					
				} else {
					fprintf(stderr, "how did we get here?\nThe string in the pipe: %s\n", line);
				}
				printf("\n\n---------------------------\n\n");
			}
		} else {
			// in child process, wait until deadlock process allows to proceed
			op.sem_num = process_id;
			op.sem_op = -1;
			op.sem_flg = 0;
			error = semop(sid3, &op, 1);
			if (error == -1) {
				fprintf(stderr, "\nsemop error(27) is: %s\n", strerror(errno));
			}

			// read the current time from the pipe
			read((fd[process_id])[0], line, len);
			sscanf(line, "%d", &time);

			char* string = deque_instruction(&processes[process_id].proc_instructions);
			while (string != NULL) {
				int icode = parse_instruction(string);
				printf("string(%d): %s", process_id, string);

				// check if any deadline misses occurred
				if (time > processes[process_id].deadline) {
					processes[process_id].deadline_misses++;
				}

				// storing all previous declarations so that the gcc doesn't throw a fit
				int n_of_names_resrc = -1;
				char* tok;
				char* str_to_send;
				int idx;
				int status_code;

				if (icode == 1) {
					// 1 is calculate
					sscanf(string, "calculate(%d)", &instr_time);
					time += instr_time;
					processes[process_id].remaining_computation_time -= instr_time;

					// the format of the string is: code time remaining_comp_time
					str_to_send = strdup("1 ");
					strcat(str_to_send, numb_to_string(time));
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(processes[process_id].remaining_computation_time));

					printf("string to send: %s\n", str_to_send);

					// send the message to deadlock process
					write((fd[n])[1], str_to_send, strlen(str_to_send));
					
					// signal the commanding process to read
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(28) is: %s\n", strerror(errno));
					}

					// wait for commanding process to send the status code
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(30) is: %s\n", strerror(errno));
					}

				} else if (icode == 2) {
					// 2 is request
					// the assignment demands that after each request, print the state of the process
					
					// the request instruction takes 1 computation time
					time += 1;
					processes[process_id].remaining_computation_time -= 1;

					// the format of the string is: code resrc_#1 resrc_#2 ... resrc_#m time remaining_comp_time
					str_to_send = strdup("2 ");

					// load the request array into the string
					tok = strtok(strdup(string), "(,);\n");
					tok = strtok(NULL, "(,);\n");
					idx = 0;
					while (tok != NULL) {
						sscanf(tok, "%d", &processes[process_id].request_array[idx]);
						tok = strtok(NULL, "(,);\n");
						strcat(str_to_send, numb_to_string(processes[process_id].request_array[idx++]));
					}
					// append time and remaining_comp_time to the end of the string
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(time));
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(processes[process_id].remaining_computation_time));

					// send the string to the commanding process
					write((fd[n])[1], str_to_send, strlen(str_to_send));

					// signal the commanding process to read
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(29) is: %s\n", strerror(errno));
					}
					
					// wait for commanding process to send the status code
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(30) is: %s\n", strerror(errno));
					}

					// once the commanding process has allowed for the process to proceed
					// read what the commanding process has sent through the pipe
					read((fd[process_id])[0], line, len);

					printf("line: %s\n", line);

					// get the status code
					tok = strtok(line, " (,);\n");
					sscanf(tok, "%d", &status_code);

					
					if (status_code == 0) {
						// the string format is: resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... 
						int resrc_idx, n_of_names_resrc;

						char req_word[20];
						for (int i = 0; i < m; i++) {
							tok = strtok(NULL, " (,);\n");
							sscanf(tok, "%d", &resrc_idx);

							tok = strtok(NULL, " (,);\n");
							sscanf(tok, "%d", &n_of_names_resrc);

							for (int j = 0; j < n_of_names_resrc; j++) {
								tok = strtok(NULL, " (,);\n");
								sscanf(tok, "%s", req_word);
								push_resource(processes[process_id].arr_allocated_resrcs, req_word, resrc_idx);
							}
						}

					} else if (status_code == 1) {
						// store the previous string instruction
						struct instruction* temp = processes[process_id].proc_instructions;
						struct instruction* new_head = (struct instruction*)malloc(sizeof(struct instruction));
						new_head->instruction = strdup(string);
						new_head->next_instruction = temp;
						processes[process_id].proc_instructions = new_head;
					} else {
						printf("WHAT HAS HAPPENED HERE\n");
					}

					printf("\n%d -- Master string: %s\n\n\n", process_id + 1, print_master_string(&processes[process_id]));


					// signal to deadlock process that child has read response
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(31) is: %s\n", strerror(errno));
					}

					// finally, wait for the deadlock process to signal to proceed
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(32) is: %s\n", strerror(errno));
					}

				} else if (icode == 3) {
					// 3 is use_resource
					int y;
					sscanf(string, "use_resources(%d,%d)", &instr_time, &y);
					struct resources* temp = NULL;
					char* t_str;
					for (int i = 0; i < m; i++) {
						struct resource* temp = (processes[process_id].arr_allocated_resrcs[i]);
						for (int j = 0; j < y && temp != NULL; j++) {
							t_str = temp->resource_name;
							strcat(processes[process_id].master_str, strdup(" "));
							strcat(processes[process_id].master_str, strdup(t_str));	//append to master string
							temp = temp->next_resource;
							processes[process_id].n_str_master++;
						}
					}
					time += instr_time;
					processes[process_id].remaining_computation_time -= instr_time;

					// the format of the string is: code time remaining_comp_time
					str_to_send = strdup("3 ");
					strcat(str_to_send, numb_to_string(time));
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(processes[process_id].remaining_computation_time));

					printf("string to send: %s (%ld)\n",str_to_send, strlen(str_to_send));

					// send the string to the commanding process
					error = write((fd[n])[1], str_to_send, strlen(str_to_send));
					if (error == -1) {
						fprintf(stderr, "\nwrite(1) is: %s\n", strerror(errno));
					}
					
					// signal the deadlock process
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(33) is: %s\n", strerror(errno));
					}

					// finally, wait for the deadlock process to signal to proceed
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(34) is: %s\n", strerror(errno));
					}

				} else if (icode == 4) {
					// 4 is release
					// send code and update time
					time += 1;
					processes[process_id].remaining_computation_time -= 1;
					str_to_send = strdup("4 ");

					// interpret the release instruction arguments into the string
					// the string format is: code resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... time
					tok = strtok(strdup(string), "(,);\n");
					tok = strtok(NULL, "(,);\n");
					idx = 0;
					while (tok != NULL) {
						sscanf(tok, "%d", &n_of_names_resrc);
						tok = strtok(NULL, "(,);\n");
						strcat(str_to_send, numb_to_string(n_of_names_resrc));
						
						for (int j = 0; j < n_of_names_resrc; j++) {
							strcat(str_to_send, strdup(" "));
							strcat(str_to_send, strdup(pop_resource(processes[process_id].arr_allocated_resrcs, idx)));
						}
						idx++;
					}
					
					// append time and remaining computation time to the end of the string
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(time));
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(processes[process_id].remaining_computation_time));

					printf("str_to_send: %s\n", str_to_send);

					// send the string to the commanding process
					error = write((fd[n])[1], str_to_send, strlen(str_to_send));
					if (error == -1) {
						fprintf(stderr, "\nwrite(1) is: %s\n", strerror(errno));
					}

					// signal the commanding process to read
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(35) is: %s\n", strerror(errno));
					}

					// wait for commanding process to signal to proceed
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(36) is: %s\n", strerror(errno));
					}

				} else if (icode == 5) {
					// 5 is print_resources_used
					// the format of the string is: 5 deadline_misses time remaining_comp_time

					printf("Print resource\n");
					// print_resource_used takes a computation time
					// the string sent is the code(5) and the current time
					time += 1;
					processes[process_id].remaining_computation_time -= 1;

					str_to_send = strdup("5 ");

					// put deadline misses
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(processes[process_id].deadline_misses));

					// append time to the end of the string
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(time));
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(processes[process_id].remaining_computation_time));
					
					// send the string to the commanding process
					error = write((fd[n])[1], str_to_send, strlen(str_to_send));
					if (error == -1) {
						fprintf(stderr, "\nwrite(1) is: %s\n", strerror(errno));
					}


					// signal the commanding process to read
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(37) is: %s\n", strerror(errno));
					}

					// wait for commanding process to signal to proceed
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(38) is: %s\n", strerror(errno));
					}

					printf("\n%d -- Master string: %s\n\n\n", process_id + 1, print_master_string(&processes[process_id]));


					// signal to deadlock process that child has read response
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(39) is: %s\n", strerror(errno));
					}

					// finally, wait for the deadlock process to signal to proceed
					op.sem_num = process_id;
					op.sem_op = -1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(40) is: %s\n", strerror(errno));
					}

				} else if (icode == 6) {
					// 6 is end
					// with end process, no computation time is taken, and all resources are released
					str_to_send = strdup("6 ");


					// it is NOT the same thing as print_resource_used, but instead, more like a release
					// the string format is: code resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... time remaining_comp_time

					for (int i = 0; i < m; i++) {
						// append the resource_type to the string
						strcat(str_to_send, numb_to_string(i));
						strcat(str_to_send, strdup(" "));
						
						int n_of_resrc;
						char* str = pop_resource(processes[process_id].arr_allocated_resrcs, i);
						char* m_str = strdup(" ");
						while (str != NULL) {
							strcat(m_str, str);
							strcat(m_str, strdup(" "));
							str = pop_resource(processes[process_id].arr_allocated_resrcs, i);
						}
						strcat(str_to_send, m_str);
					}

					// append time and remaining computation time
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(time));
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(processes[process_id].remaining_computation_time));
					/*
					if (process_id == 0) {
						printf("string to send is: %s\n", str_to_send);
					}*/

					// send the string to the commanding process
					write((fd[n])[1], str_to_send, strlen(str_to_send));

					// signal the commanding process to read
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(41) is: %s\n", strerror(errno));
					}
					printf("End process\n");

					exit(EXIT_SUCCESS);
				} else {
					// exit the process; however, print out an error and exit with FAILURE
					fprintf(stderr, "Wrong n returned\n");
					
					str_to_send = strdup("6 ");
					// the string format is: code resrc_idx1 #_of_names_resrc_1 resrc_id1_name1 resrc_id1_name2 ... resrc_idx2 #_of_names_resrc_2 resrc_id2_name1 resrc_id2_name2 ... ... time remaining_comp_time
					for (int i = 0; i < m; i++) {
						// append the resource_type to the string
						strcat(str_to_send, numb_to_string(i));
						strcat(str_to_send, strdup(" "));
						
						int n_of_resrc;
						char* str = pop_resource(processes[process_id].arr_allocated_resrcs, i);
						char* m_str = strdup(" ");
						while (str != NULL) {
							strcat(m_str, str);
							strcat(m_str, strdup(" "));
							str = pop_resource(processes[process_id].arr_allocated_resrcs, i);
						}
						strcat(str_to_send, m_str);
					}

					// append time and remaining computation time
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(time));
					strcat(str_to_send, strdup(" "));
					strcat(str_to_send, numb_to_string(processes[process_id].remaining_computation_time));
					/*
					if (process_id == 0) {
						printf("string to send is: %s\n", str_to_send);
					}*/

					// send the string to the commanding process
					write((fd[n])[1], str_to_send, strlen(str_to_send));

					// signal the commanding process to read
					op.sem_num = n;
					op.sem_op = 1;
					op.sem_flg = 0;
					error = semop(sid3, &op, 1);
					if (error == -1) {
						fprintf(stderr, "\nsemop error(42) is: %s\n", strerror(errno));
					}
					
					exit(EXIT_FAILURE);
				}

				string = deque_instruction(&processes[process_id].proc_instructions);
			}

			// process has finished executing all the strings
			fprintf(stderr, "The process should have not gotten here\n");
			exit(EXIT_FAILURE);
		}

		// free the file descriptors
		for (int i = 0; i <= n; i++) {
			free(fd[i]);
		}
		free(fd);

		op.sem_num = 0;
		op.sem_op = 1;
		op.sem_flg = 0;
		error = semop(sid1, &op, 1);
		if (error == -1) {
			fprintf(stderr, "\nsemop error(43) is: %s\n", strerror(errno));
		}




	}

	for (int i = 0; i < n; i++) {
		free(max[i]);
		free(allocation[i]);
		free(processes[i].request_array);
	}
	free(max);
	free(available);
	free(allocation);

	semctl(sid1, 0, IPC_RMID, 0);
	semctl(sid3, 0, IPC_RMID, 0);

	exit(EXIT_SUCCESS);

	return 0;
}

