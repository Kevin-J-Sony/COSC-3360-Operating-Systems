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

/*
*	This method takes in a string from the precedence graph line and initializes
*	input variables. It does not fill in these variables initially, but does it later.
*/
void init_input_var(char* input_line, int* array, int* size) {
	// Find the number of input variables given the input_line.
	// This is done by finding the number of commas splitting the
	// variables apart and adding 1 to it.
	int n_elements = 1;
	for (int i = 0; input_line[i] != '\0'; i++) {
		if (input_line[i] == ',') n_elements++;
	}
	*size = n_elements;

	array = (int *)malloc(sizeof(int) * n_elements);
	// initialize input_variables
	for (int i = 0; i < n_elements; i++) {
		array[i] = 0;
	}
	printf("%s\n", input_line);
}

void init_internal_var(char* internal_line, int* array, int* size) {
	// Find the number of internal variables given internal_line.
	// This is done by finding the number of commas splitting the
	// variables apart and adding 1 to it.
	int n_elements = 1;
	for (int i = 0; internal_line[i] != '\0'; i++) {
		if (internal_line[i] == ',') n_elements++;
	}
	*size = n_elements;

	array = (int *)malloc(sizeof(int) * n_elements);
	// initialize input_variables
	for (int i = 0; i < n_elements; i++) {
		array[i] = 0;
	}
	printf("%s\n", internal_line);
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
*   to acquire the desired result
*/
int main(int argc, char** argv) {
	/*
	*   Store the input variables and internal variables in an
	*	pointer to an array. Store the number of input and internal
	*	variables in a seperate data type.
	*/
	int* input_var;
	int* internal_var;
	int n_input_var;
	int n_internal_var;

	/*
	*	Read the first two lines of the file for the precedence graph
	*	to determine the number of input variables as well as internal variables.
	*/
	FILE* prec_graph = fopen(argv[1], "r");
	FILE* input = fopen(argv[2], "r");

	if (prec_graph == NULL || input == NULL) {
		printf("The files are null\n");
		exit(0);
	}

    
	char* line = NULL;
	size_t len = 0;
	getline(&line, &len, prec_graph);
	init_input_var(line, input_var, &n_input_var);
	line = NULL;
	len = 0;
	getline(&line, &len, prec_graph);
	init_internal_var(line, internal_var, &n_internal_var);

	return 0;
}