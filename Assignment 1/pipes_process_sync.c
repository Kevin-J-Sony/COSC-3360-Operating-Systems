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
	// Find the number of input variables given the input_line.
	// This is done by finding the number of commas splitting the
	// variables apart and adding 1 to it.
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
	// Find the number of internal variables given internal_line.
	// This is done by finding the number of commas splitting the
	// variables apart and adding 1 to it.
	int n_elements = 1;
	for (int i = 0; internal_line[i] != '\0'; i++) {
		if (internal_line[i] == ',') n_elements++;
		printf("%c", internal_line[i]);
	}
	*size = n_elements;\

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
	*   If the files are not provided, return
	*/
	if (prec_graph == NULL || input == NULL) {
		printf("The files are null\n");
		exit(0);
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
	*	10 semaphores, although some might not be needed. The semaphores serve as a
	*	way to restrict access to the internal variables.
	*	
	*	Furthermore, create 10 pipes, each pipe interacting strictly between the parent
	*	and child process, and never between child processes.
	*/
	struct sembuf res[10];
	int sem = semget(2077318, 1, 0666 | IPC_CREAT);
	int fds_to_child[10][2];
	int fds_to_parent[10][2];
	for (int i = 0; i < 10; i++) {
		pipe(fds_to_child[i]);
		pipe(fds_to_parent[i]);
	}

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

	if (pid) {
		// parent process reads the files and pipes to different processes
		while (!feof(prec_graph)) {
			fgets(sec_line, len, prec_graph);
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

			// printf("%d:\t", idx);
			if (idx != 4) {
				for (int i = 0; i < n_internal_var; i++) {
					if (strcmp(internal_var[i].name, ray[1]) == 0) {
						// normally, this is where you would pipe
						// however, in this case, we want to get the read of the next action
						// pipe(); // write to process taking care of internal_var[i]
						break;
					}
				}
			} else {
				for (int i = 0; i < n_internal_var; i++) {
					if (strcmp(internal_var[i].name, ray[1]) == 0) {
						// normally, this is where you would pipe
						// however, in this case, we want to get the read of the next action
						// pipe(); // write to process taking care of internal_var[i]
						break;
					}
				}
			}
		}

		char* msg = "This is a Hello World from parent process\n";

		close((fds_to_child[0])[0]); // close read end of pipe since parent process is not finished writing to child
		write((fds_to_child[0])[1], msg, strlen(msg));
		close((fds_to_child[0])[1]); // close write end of pipe since parent process is finished writing to child
	} else {
		close((fds_to_child[0])[1]); // close write end of pipe since child process is not finished reading from parent
		char buf[44];
		read((fds_to_child[0])[0], &buf, 44);
		write(STDOUT_FILENO, &buf, 44);
		/*
		while (read((fds_to_child[0])[0], &buf, 44) > 0) {
			write(STDOUT_FILENO, &buf, 1);
		}*/
		write(STDOUT_FILENO, "\n", 1);
		close((fds_to_child[0])[0]); // close read end of pipe since child process is finished reading from parent

		printf("process_id: %d\t%s\n", child_process_id, internal_var[child_process_id].name);
	}

	fclose(prec_graph);


	if (!pid) {
		exit(0);
	}

	return 0;
}