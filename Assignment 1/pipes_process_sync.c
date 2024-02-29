/*
*	This program is the single process version of the precedence flow. It does not
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
*   the format for strings are known, use sscanf instead.
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
		printf("%c", input_var_line[i]);
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
		printf("%c", internal_line[i]);
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
	size_t len = 100;
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
	*	Furthermore, create 10 pipes, each pipe for communication from the parent to a
	*	child process. Create another 10 pipes for communication from parent process to
	*	child process.
	*/
	int sid = semget(2077318, 10+1, 0666 | IPC_CREAT);
	int fds_to_child[10][2];
	int fds_to_parent[10][2];
	for (int i = 0; i < 10; i++) {
		pipe(fds_to_child[i]);
		pipe(fds_to_parent[i]);
	}

	int value = semctl(sid, 0, GETVAL, 0);
	printf("The value of the semaphore: %d\n", value);

	/*
	*	The value of the semaphore indicating the parent process should be 1, to allow
	*	for two processes 
	*/
	struct sembuf sb;	// this is a structure that indicate the operations to be done
	sb.sem_num = 0;		// this is the semaphore the operations will be done on
	sb.sem_op = 1;		// this is the operation done on the semaphore (incrementing by 1)
	sb.sem_flg = 0;		// this is the flag set on the operation, 0 since it is standard
	semop(sid, &sb, 1);	// only 1 operation is being done, so a function is called

	/**
	*	Identify the parent process with -1, and the child process with values from 0 to
	*	one short of the number of internal variables (n_internal_var - 1). The reason for this
	*	method of identification is the ease in which writing the code for child processes
	*	become.
	*/
	int child_process_id = -1;
	int pid = fork();
	if (!pid) {
		child_process_id = 0;
	}
	for (int i = 1; i < n_internal_var; i++) {
		if (pid) {
			pid = fork();
			if (!pid) {
				child_process_id = i;
			}
		}
	}

	value = semctl(sid, 0, GETVAL);
	printf("The value of the semaphore: %d\n", value);

	if (pid) {
		/*
		*	Indicate that the parent process is being partially occupied by reading the file, and
		*	so that the "number of parent processes" available to send info to is decreased by one.
		*	
		*	The reason for this is because once the parent process finishes reading the files and
		*	sending the information to the respective child processes, there should be some way
		*	for the child process to know that it no longer needs to keep on "listening" to what
		*	the parent has to say.
		*/
		sb.sem_num = 0;
		sb.sem_op = -1;
		sb.sem_flg = 0;
		semop(sid, &sb, 1);
		/*
		*	The parent process reads the precedence graph file line by line
		*	and sends the result to the child process.
		*/
		while (!feof(prec_graph)) {
			fgets(sec_line, len, prec_graph);
			strcpy(line, sec_line);
			// if sec_line is "write(x, y, z, ...);", break out of the loop
			if (sec_line[0] == 'w')
				break;
			
			// otherwise, get the string of the last variable to assign it to
			// the corresponding process
			char* tok = strtok(sec_line," ,;\n");
			int n_char = 3;
			char cur_int_var[n_char];
			int idx = 0;

			while (tok != NULL) {
				strcpy(cur_int_var, tok);
				tok = strtok(NULL, " ,;\n");
			}

			for (int i = 0; i < n_internal_var; i++) {
				if (strcmp(internal_var[i].name, cur_int_var) == 0) {
					// send a signal that the parent process is occupied
					sb.sem_num = 0;
					sb.sem_op = -1;
					sb.sem_flg = 0;
					semop(sid, &sb, 1);

					// close the read end of the pipe since parent is not finished writing
					// and so child cannot read yet
					close((fds_to_child[i])[0]);

					// write in the pipe to pass to the child process
					// responsible for the internal_variable
					write((fds_to_child[i])[1], line, len);
					printf("%s\n", line);

					// close the write end of the pipe since parent is finished writing
					// and so parent cannot write
					close((fds_to_child[i])[1]);

					// send a signal that the parent process is no longer occupied
					sb.sem_num = 0;
					sb.sem_op = 1;
					sb.sem_flg = 0;
					semop(sid, &sb, 1);

					// exit the loop
					break;
				}
			}
		}
		
		/*
		*	Now that the parent has finished reading the precedence graph, indicate this increasing
		*	the value of the semaphore corresponding to the parent process. This serves as a way to
		*	"signal" to the children that the parents are done.
		*/
		sb.sem_num = 0;
		sb.sem_op = 1;
		sb.sem_flg = 0;
		semop(sid, &sb, 1);

	} else {
		/*
		*	While the parent process is reading from the file, the child process should not leave the loop.
		*	In the case that the parent signals, it means that it has finished reading the file, and is ready for
		*	the child processes to finish their tasks and pipe the values it contains back to it to store it.
		*	
		*	If the parent process signals while the child process is still in the loop, the child can safely
		*	finish up parsing the line and modifying the value and exit the loop. 
		*/
		while(1) {			
			// send a signal that this internal variable is occupied
			sb.sem_num = child_process_id + 1;
			sb.sem_op = -1;
			sb.sem_flg = 0;
			semop(sid, &sb, 1);

			// close write end of pipe since child process is not finished reading from parent
			close((fds_to_child[0])[1]);

			read((fds_to_child[child_process_id + 1])[0], line, len);

			// close read end of pipe since child process is finished reading from parent
			close((fds_to_child[0])[0]);

			printf("\nprocess_id: %d\t[%s]\n", child_process_id + 1, line);


			strcpy(line, sec_line);
			char* tok = strtok(sec_line," ,;\n");
			int n_elements = 4;
			int n_char = 3;
			char ray[n_elements][n_char];
			int idx = 0;

			while (tok != NULL) {
				strcpy(ray[idx++], tok);
				tok = strtok(NULL, " ,;\n");
			}

			if (idx != 4) {
				printf("%s\n", ray[2]);
				for (int i = 0; i < n_internal_var; i++) {
					if (strcmp(internal_var[i].name, ray[2]) == 0) {
						// close the read end of the pipe
						close((fds_to_child[i])[0]);

						// write in the pipe to pass to the child process
						// responsible for the internal_variable
						write((fds_to_child[i])[1], line, len);

						// close the write end of the pipe
						close((fds_to_child[i])[1]);
						break;
					}
				}
			} else {
				printf("%s\n", ray[3]);
				for (int i = 0; i < n_internal_var; i++) {
					if (strcmp(internal_var[i].name, ray[3]) == 0) {
						break;
					}
				}
			}

			// send a signal that this internal variable is free
			sb.sem_num = child_process_id + 1;
			sb.sem_op = 1;
			sb.sem_flg = 0;
			semop(sid, &sb, 1);

		}
	}

	if (!pid) {
		exit(EXIT_SUCCESS);
	}
	fclose(prec_graph);


	semctl(sid, 0, IPC_RMID, 0);

	return 0;
}