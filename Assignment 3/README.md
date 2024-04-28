
To compile this program, run:

	gcc -Wall -g3 -fsanitize=address auxillary_structures.c virtual_mem_manager.c -o main
	gcc -g auxillary_structures.c virtual_mem_manager.c -o main

The quirks of the paging policies are all described above the functions in which they are executed in. Analysis of the some of them will be described here.

OPT-X Paging Policy performs not as expected since it can only look X time into the future, which makes it replace a page over another which happens to appear before it, simply because both were
outside the field of view of this algorithm. Making the OPT algorithm would be a simple fix, but the assignment doesn't ask for OPT, but OPT-X. One example where not being able to see further into the
future makes OPT-X perform worse than other algorithms is LIFO with Assignment3_Input.txt.

To see the main memory stack in the execution, set DEBUG_MODE to TRUE in virtual_mem_manager.c.

Forks and semaphores were not used in this program since using it was unnecessary

stack: 
stack: 0 
stack: 1 0 
stack: 1 0 
stack: 15 1 0 
stack: 2 15 1 0 
stack: 6 2 15 1 0 
stack: 3 6 2 15 1 0 
stack: 3 6 2 15 1 0 
stack: 3 6 2 15 1 0 
stack: 3 6 2 15 1 0 
stack: 7 3 6 2 15 1 0 
stack: 7 3 6 2 15 1 0 
stack: 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 13 5 7 3 6 2 15 1 0 
stack: 13 5 7 3 6 2 15 1 0 
stack: 13 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 10 5 7 3 6 2 15 1 0 
stack: 13 5 7 3 6 2 15 1 0 
stack: 13 5 7 3 6 2 15 1 0 
stack: 13 5 7 3 6 2 15 1 0 
stack: 13 5 7 3 6 2 15 1 0 
stack: 13 5 7 3 6 2 15 1 0 
stack: 13 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 
stack: 4 5 7 3 6 2 15 1 0 

The stack grows to the left, and it contains the page numbers