/*
*	This program simulates deadlock avoidance using the Banker's algorithm
*   combined with Earliest-Deadline-First scheduling (using Longest-Job-First as tie-breaker)
*   and Least-Laxity-First scheduingling (using Shortest-Job-First as tie-breaker)
*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
	/*
    *   There is a single argument provided as input. It has two integers, m and n,
    *   in the first two lines, signifying the number of resources and the number
    *   of processes respectively. The format then splits into two different possibilities:
    *       - The next m and n*m lines are integers to be filled into the AVALIABLE array and MAX
    *         matrix respectively.
    *       - The next line after the integers is a list containing m integers to be put into AVAILABLE
    *         array, and n lines containing m integers to be put into the MAX matrix.
    *   Afterwards, the following lines contain instructions to simulate how the processes will use
    *   the resources.
	*/
    int n, m;
	FILE* input = fopen(argv[1], "r");

	/*
	*   If the files are not provided, exit the program
	*/
	if (input == NULL) {
		printf("The file is null\n");
		exit(EXIT_SUCCESS);
	}

    /*
	*	The lines will not exceed 300 characters, so allocate 300 characters as a buffer.
	*	Read the first two integers, initialize the AVALIABLE and MAX data structures, as well as
    *   any other data structures necessary, and then read the next line.
	*/
	size_t len = 300;
	char line[len];
	fgets(line, len, input);
    sscanf(line, "%d", &m);
	fgets(line, len, input);
	sscanf(line, "%d", &n);

    int* avaliable = (int*)malloc(sizeof(int) * m);
    int** max = (int**)malloc(sizeof(int*) * n);

    for (int i = 0; i < n; i++) {
        max[i] = (int*)malloc(sizeof(int) * m);
    }

    // if the next line read starts with an 'a', the first format is used
    fgets(line, len, input);
    if (line[0] == 'a') {
        int t, t1;
        for (int i = 0; i < m; i++) {
            fgets(line, len, input);
        	sscanf(line, "available[%d]=%d", &t, &available[i]);
        }
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                fgets(line, len, input);
            	sscanf(line, "max[%d,%d]=%d", &t, &t1, &max[i][j]);
            }
        }
        
    }

	fclose(input);

    // test to see if input is read
    for (int i = 0; i < m; i++) {
    	printf("%d\t", avaliable[i]);
    }
    printf("\n\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
        	printf("%d\t", max[i][j]);
        }
        printf("\n");
    }


    for (int i = 0; i < n; i++) {
        free(max[i]);
    }
    free(max);
    free(available);

    return 0;
}

