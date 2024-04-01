/*
*	This program simulates deadlock avoidance using the Banker's algorithm
*   combined with Earliest-Deadline-First scheduling (using Longest-Job-First as tie-breaker)
*   and Least-Laxity-First scheduing (using Shortest-Job-First as tie-breaker). 
*   
*   It is an experiment to determine which scheduling works best with the Banker's Algorithm
*
*	C++ is used instead of C since the standard library is equipped with the queue data structure and
*	so it would be unnecessary to reinvent the wheel in this regard.
*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <queue>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>

struct process {
	int process_number;
	int deadline;
	int computation_time;

	std::vector<char*> instructions;
	int current_idx;	// during simulation, current_idx points to the current instruction running
};

bool safe_state(int n, int m, int* available, int** need, int** allocation) {
	// need + allocation = max
	int* work = (int*)malloc(sizeof(int) * m);
	bool* finish = (bool*)malloc(sizeof(bool) * n);
	for (int i = 0; i < m; i++) {
		work[i] = available[i];
	}
	for (int i = 0; i < n; i++) {
		finish[i] = false;
	}
	int flag = false;
	while (!flag) {
		int flag3 = false;
		for (int i = 0; i < n; i++) {
			if (!finish[i]) {
				flag3 = true;
				int flag2 = true;
				for (int j = 0; j < m; j++) {
					flag2 = flag2 && need[i][j] <= work[j];
				}
				if (flag2) {
					for (int j = 0; j < m; j++) {
						work[j] = work[j] + allocation[i][j];
					}
					finish[i] = true;
				}
			}
		}
		// if flag3 is not flagged at all, it would remain false and so set flag to true
		flag = !flag3;
	}

	for (int i = 0; i < n; i++) {
		if (!finish[i]) {
			return false;
		}
	}
	return true;
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
	*   in the first two lines, signifying the number of resources and the number
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

	std::ifstream input2;
	input2.open(argv[1]);

	std::ofstream edloutput;

	/*
	*   If the files are not provided, exit the program
	*/
	if (input == NULL) {
		printf("The file is null\n");
		exit(EXIT_FAILURE);
	}

	if (!input2.is_open()) {
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

	int* available = (int*)malloc(sizeof(int) * m);
	int** max = (int**)malloc(sizeof(int*) * n);
	int** allocation = (int**)malloc(sizeof(int*) * n);
	struct process* processes = (struct process*)malloc(sizeof(struct process) * n);

	for (int i = 0; i < n; i++) {
		max[i] = (int*)malloc(sizeof(int) * m);
		allocation[i] = (int*)malloc(sizeof(int) * m);
	}


	for (int i = 0; i < m; i++) {
		fscanf(input, "%d", &available[i]);
	}
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			fscanf(input, "%d", &max[i][j]);
			allocation[i][j] = 0;
		}
	}

	// there should only need to be one instance of struct sembuf
	// since this is a data structure to send information to the linux
	// operation to change the semaphore itself, not
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

	// read the instructions from the file
	// while the first line of the input after reading the available and
	// max matrices should be process_#, we could have a whitespace throw it off
	int proc_count = 0;
	while (!feof(input)) {
		fgets(line, len, input);
		strncpy(sec_line, line, strlen("process"));
		if (strcmp("process", sec_line) == 0) {
			// we have the start to a process
			// the next two lines give the deadline and computation time
			processes[proc_count].process_number = proc_count;
			fscanf(input, "%d", &processes[proc_count].deadline);
			fscanf(input, "%d", &processes[proc_count].computation_time);

			int terminate = (1 == 0);
			printf("PROCESS COUNT: %d\n", proc_count);
			while (!feof(input) && !terminate) {
				fgets(line, len, input);
				strcpy(sec_line, line);
				if (strcmp("end.\n", sec_line) == 0) {
					terminate = (1 == 1);
				} else {
					// store the instructions in the vector					
					processes[proc_count].instructions.push_back(strdup(line));
				}
				//printf("%d\t%s\n", strcmp("end.\n", sec_line), sec_line);
				printf("%s\n", processes[proc_count].instructions[processes[proc_count].instructions.size() - 1]);
			}
		}

	}


	fclose(input);

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
	}
	free(max);
	free(available);

	return 0;
}

