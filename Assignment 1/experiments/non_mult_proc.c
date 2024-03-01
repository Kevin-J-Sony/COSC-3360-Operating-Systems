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
	
	size_t len = 100;
	char line[len];
	char sec_line[len];
	fgets(line, len, prec_graph);
	fgets(sec_line, len, input);
	init_input_var(line, sec_line, input_var, &n_input_var);
	fclose(input);

	fgets(line, len, prec_graph);
	init_internal_var(line, internal_var, &n_internal_var);

	// parent process reads the files and pipes to different processes
	while (!feof(prec_graph)) {
		fgets(sec_line, len, prec_graph);
		strcpy(line, sec_line);
		char* tok = strtok(sec_line," ,;\n");
		int n_elements = 4;
		int n_char = 4;
		char ray[n_elements][n_char];
		int idx = 0;

		while (tok != NULL) {
			strcpy(ray[idx++], tok);
			tok = strtok(NULL, " ,;\n");
		}

		printf("%d:\t", idx);
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

		for (int i = 0; i < n_elements; i++) {
			printf("%s, ", ray[i]);
		}
		printf("\n");
	}



	return 0;
}