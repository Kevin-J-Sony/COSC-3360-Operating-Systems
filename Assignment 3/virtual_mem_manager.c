#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


/*
*
*/
struct frame {
    int page_number;
    int frame_number;
    int valid;
    int last_used;
};

int main(int argc, char** argv) {
	FILE* input = fopen(argv[1], "r");
	size_t len = 1000;
	char line[len];
	char sec_line[len];


	/*
	*   If the files are not provided, exit the program
	*/
	if (input == NULL) {
		printf("The file is null\n");
		exit(EXIT_FAILURE);
	}

	/*
	*	The lines will not exceed 1000 characters, so allocate 1000 characters as a buffer.
	*	Read the first seven integers, and put them into the relevant variables
	*/
    int tp, ps, r, X, min, max, k;
	fgets(line, len, input);
	sscanf(line, "%d", &tp);

	fgets(line, len, input);
	sscanf(line, "%d", &ps);

	fgets(line, len, input);
	sscanf(line, "%d", &r);

	fgets(line, len, input);
	sscanf(line, "%d", &X);

	fgets(line, len, input);
	sscanf(line, "%d", &min);

	fgets(line, len, input);
	sscanf(line, "%d", &max);

	fgets(line, len, input);
	sscanf(line, "%d", &k);

    /*
    *   Initialize k processes.
    *
    */
   	while (!feof(input)) {
		fgets(line, len, input);
		sscanf(line, "%d %d", &k, &dummy);

		
	}

    return 0;
}