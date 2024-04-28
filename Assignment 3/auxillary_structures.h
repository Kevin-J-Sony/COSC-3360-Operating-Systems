

/*
*	Create a linked list for instructions. Elements are added to the end of the linked list. The parent process will parse the instructions and send
*   it to the child process to be read. I6t cannot continue from the child process until all the child processes for a given algorithm have finished execution.
*
*   Use an integer to determine if an instruction has been "read" (executed).
*/
struct instruction {
	int pid;
	int addr;
	struct instruction* next_instruction;
	struct instruction* prev_instruction;

	int read;
};


/*
*   Each process contains its process id, the number of pages it requires, and the current amount of page faults it generated
*
*	The structure contains the working set structure for the working set page policy as well as information if it is currently running.
*/
struct process {
	int pid;
	int number_of_pages;
	int number_of_page_faults;

	struct frame* working_set;
	int currently_running;
};


/*
*	A doubly linked list to store the page number in a frame. It also contains the amount of times it has been accessed (while in main memory),
*	as well as the time since it has been accessed.
*/
struct frame {
	struct frame* previous_frame;
	struct frame* next_frame;
	int page_number;

	int frequency;
	int time;

	// For the working set page policy, store the last process which has accessed it
	int last_pid;
};

/*
*   This structure encapsulates the frame structure and stores the maximum size allowed as well as the current size of the frame. 
*/
struct memory {
	int max_size;
	int size;
	struct frame* memory_head;
};
