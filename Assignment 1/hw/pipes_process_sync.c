/*
*	This program is the multiprocess version of the precedence flow. It does not
*	use semaphores or pipes to different processes, and the sole intention is to provide
*	the framework on which the multiprocessor program can synchronize off of.
*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
*   Create a struct that holds in the variable and the current value it possesses.
*	The assumptions from the input demand that only 3 characters are needed to identify
*	the variable.
*/
struct variable {
	char name[3];
	int value;
};

/*
*	This method takes in a line from the precedence graph as well as a line from the input 
*	file and initializes an array of input variables.
*
*	This part of the code should not be accessed or manipulated by child processes. The reason
*	for this is because it uses the strtok function provided by the standard library, and
*	documentation as well as other online sources warned of its volatility. In cases where
*	the format for strings are known, use sscanf instead.
*/
void init_input_var(char* input_var_line, char* input_line, struct variable* array, int* size) {
	/*
	*	Find the number of input variables given input_line.
	*	This is done by finding the number of commas splitting the
	*	variables apart and adding 1 to it.
	*/
	int n_elements = 1;
	for (int i = 0; input_var_line[i] != '\0'; i++) {
		if (input_var_line[i] == ',') n_elements++;
	}
	*size = n_elements;
	
	int idx = 0;
	char* tok = strtok(input_line," ,;");
	while (tok != NULL) {
		array[idx++].value = atoi(tok);
		tok = strtok(NULL, " ,");
	}

	idx = 0;
	tok = strtok(input_var_line, " ,;");
	tok = strtok(NULL, " ,;");
	while (tok != NULL) {
		strcpy(array[idx++].name, tok);
		tok = strtok(NULL, " ,;");
	}
}

/*
*	This method takes in a line from the precedence graph file and initializes an 
*	array of internal variables.
*
*	Like in the method above, this part of the code should not be accessed or manipulated 
*	by child processes, although, it should not be plagued by the same problems.
*/
void init_internal_var(char* internal_line, struct variable* array, int* size) {
	/*
	*	Find the number of internal variables given internal_line.
	*	This is done by finding the number of commas splitting the
	*	variables apart and adding 1 to it.
	*/
	int n_elements = 1;
	for (int i = 0; internal_line[i] != '\0'; i++) {
		if (internal_line[i] == ',') n_elements++;
	}
	*size = n_elements;

	int idx = 0;
	char* tok = strtok(internal_line," ,;");
	tok = strtok(NULL, " ,;");
	while (tok != NULL) {
		array[idx].value = 0;
		strcpy(array[idx++].name, tok);
		tok = strtok(NULL, " ,;");
	}
}

/*
*	This program takes in two arguments from terminal given by the user.
*	
*	The first argument is the file to precedence graph, and the second argument
*	is the file to the inputs of the precedence graph.
*
*	The program outputs a file which contains the input variable and the 
*	internal variables after it has gone through the precedence graph.
*
*	The way this program handles the code is going step by step through the files
*   to acquire the desired result.
*
*   The following assumptions were made about the input provided:
*       - The internal variables would be enumerated in order from p0, and go to p9 at max,
*         and so would constitute two characters.
*       - The input variables would be enumerated in order from a, and go to to j at max, and so
*         would constitute a single character.
*       - Every line of the arguments would be partitioned by a semicolon and a newline
*       - The first two lines are the input and internal variables while the last one is an
*         instruction to write to output.
*
*   These assumptions are necessary to reduce the size of the data structures. A structure that
*   holds data for the names of the characters would require the variable names to be changed, 
*   or it would require that in reading the file, the names of the variables would change
*   to a format more easily accessible.
*/
int main(int argc, char** argv) {
	/*
	*   Store the input variables and internal variables in an
	*	pointer to an array. Store the number of input and internal
	*	variables in a seperate data type.
	*/
	struct variable input_var[10];
	struct variable internal_var[10];
	int n_input_var;
	int n_internal_var;

	/*
	*	Read the first two lines of the file for the precedence graph
	*	to determine the number of input variables as well as internal variables.
	*/
	FILE* prec_graph = fopen(argv[1], "r");
	FILE* input = fopen(argv[2], "r");

	/*
	*   If the files are not provided, exit
	*/
	if (prec_graph == NULL || input == NULL) {
		printf("The files are null\n");
		exit(EXIT_SUCCESS);
	}
	
	/*
	*	The lines will not exceed 100 characters, so allocate 100 characters as a buffer.
	*	Initialize the variable arrays.
	*/
	size_t len = 300;
	char line[len];
	char sec_line[len];
	fgets(line, len, prec_graph);
	fgets(sec_line, len, input);
	init_input_var(line, sec_line, input_var, &n_input_var);
	fclose(input);

	fgets(line, len, prec_graph);
	init_internal_var(line, internal_var, &n_internal_var);

	/*
	*	With the input variables and internal variables defined, initialize
	*	10+1 semaphores, the extra semaphore being for the parent process. The 
	*	semaphores serve as a way to restrict access to the internal variables.
	*	
	*	With pipes, closing them means that there is no way to reopen them, and a new
	*	pipe would have to be constructed to send over the data. Associate each pipe
	*	with a process, so that a process reads from a pipe, and other processes
	*	write in the pipe of the process they wish to communicate to
	*/
	// int sid = semget(2077318, 10+1, 0666 | IPC_CREAT);
	struct sembuf op[10 + 1];
	// int sid = semget(2077318, 1, 0666 | IPC_CREAT);
	int sid = semget(2077318, 10+1, 0666 | IPC_CREAT);
	
	int fd_of_proc[10+1][2];
	for (int i = 0; i < 10+1; i++) {
		pipe(fd_of_proc[i]);
	}

	/**
	*	Identify the parent process with -1, and the child process with values from 0 to
	*	one short of the number of internal variables (n_internal_var - 1). The reason for this
	*	method of identification is the ease in which writing the code for child processes
	*	become.
	*
	*	It's a crucial way to classify the different processes, important for the management
	*	of different semaphores and pipes. The parent process has an ID of -1, and is associate
	*	index 0 for semaphores and pipes, and the general formula to get the index of the
	*	the associate pipe/semaphore for a process is child_process + 1.
	*/
	int child_process_id = -1;
	int pid = 1;
	for (int i = 0; i < n_internal_var; i++) {
		if (pid) {
			pid = fork();
			if (!pid) {
				child_process_id = i;
			}
		}
	}
	char child_process_id_str[3];
	sprintf(child_process_id_str, "%d", child_process_id);

	if (pid) {
		/*
		*	The parent process reads the precedence graph file line by line
		*	and sends the result to the child process.
		*/
		struct sembuf sb;
		int terminate = (1 == 0);
		const char delimiter[] = " ,;()\n\t";
		while (!feof(prec_graph) && !terminate) {
			fgets(sec_line, len, prec_graph);
			strcpy(line, sec_line);
			
			int n_char = 3;
			char cur_int_var[n_char];
			char* tok = strtok(sec_line, delimiter);
			//printf("%s\n", line);
			// if the first token is write, end the loop
			if (strcmp(tok, "write") == 0) {
				//printf("\n\nWrite instruction read\n\n");
				terminate = (1 == 1);
				break;
			}

			while (tok != NULL) {
				strcpy(cur_int_var, tok);
				tok = strtok(NULL, delimiter);
			}

			for (int i = 0; i < n_internal_var; i++) {
				if (strcmp(internal_var[i].name, cur_int_var) == 0) {
					
					// don't close since it's impossible to tell when to open
					// close the read end of the pipe since parent is not finished writing
					// and so child cannot read yet	
					// close((fd_of_proc[i])[0]);

					// write in the pipe to pass the message to the child process
					// responsible for the internal_variable
					write((fd_of_proc[i + 1])[1], line, len);
					// furthemore, write down the process it came from
					write((fd_of_proc[i + 1])[1], child_process_id_str, 3);

					//printf("send the message: %s\n", cur_int_var);

					// close the write end of the pipe since parent is finished writing
					// and so parent cannot write
					// close((fd_of_proc[i])[1]);

					// send a signal that the child process corresponding to the internal variable
					// is now free to run the code
					op[i + 1].sem_num = i + 1;
					op[i + 1].sem_op = 1;
					op[i + 1].sem_flg = 0;
					semop(sid, &op[i + 1], 1);
					

					// wait for the child process to finish reading the pipe
					op[0].sem_num = 0;
					op[0].sem_op = -1;
					op[0].sem_flg = 0;
					semop(sid, &op[0], 1);

					// exit the loop
					break;
				}
			}
		}


		/*
		*	Send a message to all child processes to send data and to terminate
		*/
		for (int i = 0; i < n_internal_var; i++) {
			// send an instruction to terminate
			strcpy(line, "terminate\n");
			write((fd_of_proc[i + 1])[1], line, len);
			write((fd_of_proc[i + 1])[1], child_process_id_str, 3);
			// printf("sent message: %s%s\n", line, child_process_id_str);

			// signal that the child process it requests data from should read
			/*
			sb.sem_num = i + 1;
			sb.sem_op = 1;
			sb.sem_flg = 0;
			semop(sid, &sb, 1);
			*/
			op[i + 1].sem_num = i + 1;
			op[i + 1].sem_op = 1;
			op[i + 1].sem_flg = 0;
			semop(sid, &op[i + 1], 1);
			// printf("finished writing to process %d\n", i);

			// wait until process it requests data from finished reading
			/*
			sb.sem_num = child_process_id + 1;
			sb.sem_op = -1;
			sb.sem_flg = 0;
			semop(sid, &sb, 1);
			*/
			op[child_process_id + 1].sem_num = child_process_id + 1;
			op[child_process_id + 1].sem_op = -1;
			op[child_process_id + 1].sem_flg = 0;
			semop(sid, &op[child_process_id + 1], 1);
			// printf("waiting for process %d to finish reading\n", i);

			// wait for process it requests data from to finish writing
			/*
			sb.sem_num = child_process_id + 1;
			sb.sem_op = -1;
			sb.sem_flg = 0;
			semop(sid, &sb, 1);
			*/
			op[child_process_id + 1].sem_num = child_process_id + 1;
			op[child_process_id + 1].sem_op = -1;
			op[child_process_id + 1].sem_flg = 0;
			semop(sid, &op[child_process_id + 1], 1);
			// printf("waiting for process %d to finish writing\n", i);

			// read the response from the process
			char buff[15];
			read((fd_of_proc[0])[0], buff, 15);
			sscanf(buff, "%d", &internal_var[i].value);
			// printf("recieved value from process %d: %d\n", i, internal_var[i].value);
			
		}

	} else {
		/*
		*	While the parent process is reading from the file, the child process should not leave the loop.
		*	In the case that the parent signals, it means that it has finished reading the file, and is ready for
		*	the child processes to finish their tasks and pipe the values it contains back to it to store it.
		*	
		*	If the parent process signals while the child process is still in the loop, the child can safely
		*	finish up parsing the line and modifying the value and exit the loop. 
		*/
		int terminate = (1 == 0);
		do {
			struct sembuf sb;
			int value = semctl(sid, child_process_id + 1, GETVAL);
			// printf("process %d waiting... (semaphore value: %d)\n", child_process_id, value);
			// send a signal that this internal variable is waiting for the another process to write
			/*
			sb.sem_num = child_process_id + 1;
			sb.sem_op = -1;
			sb.sem_flg = 0;
			semop(sid, &sb, 1);
			*/
			op[child_process_id + 1].sem_num = child_process_id + 1;
			op[child_process_id + 1].sem_op = -1;
			op[child_process_id + 1].sem_flg = 0;
			semop(sid, &op[child_process_id + 1], 1);
			// printf("process %d permitted to go! (semaphore value: %d)\n", child_process_id, value);

			// for now, don't close the pipes since there is no way to reopen them
			// close write end of pipe since child process is not finished reading from parent
			// close((fd_of_proc[child_process_id - 1])[1]);
			char writing_process_str[3];
			read((fd_of_proc[child_process_id + 1])[0], line, len);
			read((fd_of_proc[child_process_id + 1])[0], writing_process_str, 3);

			int writing_process;
			sscanf(writing_process_str, "%d", &writing_process);
			// printf("process %d reading message sent from process %d. message: %s\n", child_process_id, writing_process, line);

			// close read end of pipe since child process is finished reading from parent
			// close((fd_of_proc[child_process_id - 1])[0]);

			/*
			*	If the message sent is explicitly a "send data" or "terminate" request, send data to the
			*	writing process or terminate. Otherwise, assume that the message is an instruction.
			*/
			if (strcmp(line, "send data\n") == 0) {
				// this is an instruction to send data to the writing process

				char buff[15];
				sprintf(buff, "%d", internal_var[child_process_id].value);
				write((fd_of_proc[writing_process + 1])[1], buff, 15);

				// signal that the child process finished response
				// and so waiting process should read
				/*
				sb.sem_num = writing_process + 1;
				sb.sem_op = 1;
				sb.sem_flg = 0;
				semop(sid, &sb, 1);
				*/
				op[writing_process + 1].sem_num = writing_process + 1;
				op[writing_process + 1].sem_op = 1;
				op[writing_process + 1].sem_flg = 0;
				semop(sid, &op[writing_process + 1], 1);
			} else if (strcmp(line, "terminate\n") == 0) {
				char buff[15];
				sprintf(buff, "%d", internal_var[child_process_id].value);
				write((fd_of_proc[writing_process + 1])[1], buff, 15);
				;

				// signal that the child process should read
				/*
				sb.sem_num = writing_process + 1;
				sb.sem_op = 1;
				sb.sem_flg = 0;
				semop(sid, &sb, 1);
				*/
				op[writing_process + 1].sem_num = writing_process + 1;
				op[writing_process + 1].sem_op = 1;
				op[writing_process + 1].sem_flg = 0;
				semop(sid, &op[writing_process + 1], 1);

				terminate = (1 == 1);
			} else{
				strcpy(sec_line, line);
				char* tok = strtok(sec_line," ,;\n\t");
				int n_elements = 4;
				int n_char = 3;
				char ray[n_elements][n_char];
				int idx = 0;

				while (tok != NULL) {
					strcpy(ray[idx++], tok);
					tok = strtok(NULL, " ,;\n\t");
				}

				if (idx != 4) {
					int internal = (1 == 1);
					int x;
					for (int i = 0; i < n_input_var; i++) {
						if (strcmp(input_var[i].name, ray[0]) == 0) {
							x = input_var[i].value;
							internal = (1 == 0);
							break;
						}
					}
					for (int i = 0; i < n_internal_var && internal; i++) {
						if (strcmp(internal_var[i].name, ray[0]) == 0) {
							// send a request for an internal variable
							strcpy(line, "send data\n");
							write((fd_of_proc[i + 1])[1], line, len);
							// furthemore, write down the process it came from
							write((fd_of_proc[i + 1])[1], child_process_id_str, 3);
							;

							// signal that process it requests data from should read
							/*
							sb.sem_num = i + 1;
							sb.sem_op = 1;
							sb.sem_flg = 0;
							semop(sid, &sb, 1);
							*/
							op[i + 1].sem_num = i + 1;
							op[i + 1].sem_op = 1;
							op[i + 1].sem_flg = 0;
							semop(sid, &op[i + 1], 1);

							// wait until process it requests data from finished reading
							/*
							sb.sem_num = child_process_id + 1;
							sb.sem_op = -1;
							sb.sem_flg = 0;
							semop(sid, &sb, 1);
							*/
							op[child_process_id + 1].sem_num = child_process_id + 1;
							op[child_process_id + 1].sem_op = -1;
							op[child_process_id + 1].sem_flg = 0;
							semop(sid, &op[child_process_id + 1], 1);

							// wait for process it requests data from to finish reading
							/*
							sb.sem_num = child_process_id + 1;
							sb.sem_op = -1;
							sb.sem_flg = 0;
							semop(sid, &sb, 1);
							*/
							op[child_process_id + 1].sem_num = child_process_id + 1;
							op[child_process_id + 1].sem_op = -1;
							op[child_process_id + 1].sem_flg = 0;
							semop(sid, &op[child_process_id + 1], 1);

							// read the response from the process
							char buff[15];
							read((fd_of_proc[child_process_id + 1])[0], buff, 15);
							sscanf(buff, "%d", &x);

							break;
						}
					}
					// printf("x = %d\n", x);
					internal_var[child_process_id].value = x;

				} else {
					int x;
					int internal = (1 == 1);
					for (int i = 0; i < n_input_var; i++) {
						if (strcmp(input_var[i].name, ray[1]) == 0) {
							x = input_var[i].value;
							internal = (1 == 0);
							break;
						}
					}
					for (int i = 0; i < n_internal_var && internal; i++) {
						if (strcmp(internal_var[i].name, ray[1]) == 0) {
							// write the response for the internal variable
							strcpy(line, "send data\n");
							write((fd_of_proc[i + 1])[1], line, len);
							// furthemore, write down the process it came from
							write((fd_of_proc[i + 1])[1], child_process_id_str, 3);

							// signal that process it requests data from should read
							/*
							sb.sem_num = i + 1;
							sb.sem_op = 1;
							sb.sem_flg = 0;
							semop(sid, &sb, 1);
							*/
							op[i + 1].sem_num = i + 1;
							op[i + 1].sem_op = 1;
							op[i + 1].sem_flg = 0;
							semop(sid, &op[i + 1], 1);

							// wait until process it requests data from finished reading
							/*
							sb.sem_num = child_process_id + 1;
							sb.sem_op = -1;
							sb.sem_flg = 0;
							semop(sid, &sb, 1);
							*/
							op[child_process_id + 1].sem_num = child_process_id + 1;
							op[child_process_id + 1].sem_op = -1;
							op[child_process_id + 1].sem_flg = 0;
							semop(sid, &op[child_process_id + 1], 1);


							// wait for process it requests data from to finish reading
							/*
							sb.sem_num = child_process_id + 1;
							sb.sem_op = -1;
							sb.sem_flg = 0;
							semop(sid, &sb, 1);
							*/
							op[child_process_id + 1].sem_num = child_process_id + 1;
							op[child_process_id + 1].sem_op = -1;
							op[child_process_id + 1].sem_flg = 0;
							semop(sid, &op[child_process_id + 1], 1);

							// read the response from the process
							char buff[15];
							read((fd_of_proc[child_process_id + 1])[0], buff, 15);
							sscanf(buff, "%d", &x);

							break;
						}
					}

					if (strcmp("+", ray[0]) == 0) {
						internal_var[child_process_id].value += x;
					} else if (strcmp("-", ray[0]) == 0) {
						internal_var[child_process_id].value -= x;
					} else if (strcmp("*", ray[0]) == 0) {
						internal_var[child_process_id].value *= x;
					} else if (strcmp("/", ray[0]) == 0) {
						internal_var[child_process_id].value /= x;
					}
				}
			}

			// send a signal to the writing process that the current process has finished reading and processing
			op[writing_process + 1].sem_num = writing_process + 1;
			op[writing_process + 1].sem_op = 1;
			op[writing_process + 1].sem_flg = 0;
			semop(sid, &op[writing_process + 1], 1);

			// printf("process %d internal variable state: %s\t%d\n", child_process_id, internal_var[child_process_id].name, internal_var[child_process_id].value);
		} while (!terminate);
		for (int i = 0; i < 10; i++) {

		}
		
	}
	fclose(prec_graph);

	for (int i = 0; i < 10+1; i++) {
		close((fd_of_proc[i])[1]);
		close((fd_of_proc[i])[0]);
	}
	if (!pid) {
		exit(EXIT_SUCCESS);
	}

	semctl(sid, 0, IPC_RMID, 0);

	for (int i = 0; i < n_internal_var; i++) {
		printf("%s -> %d\n", internal_var[i].name, internal_var[i].value);
	}

	return 0;
}
