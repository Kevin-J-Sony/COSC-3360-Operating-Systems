#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "auxillary_structures.h"


/*
*	Define TRUE and FALSE
*/
#define TRUE (1 == 1)
#define FALSE (1 == 0)

#define DEBUG_MODE FALSE

/*
*	Global variables for the number of pages, the page size, the amount of pages per process, the lookahead window for LRU_X and OPT,
*	the smallest amount of free frames allowed, the largest amount of free frames allowed, and the number of processes.
*
*	Global variable also for a list of instructions, a list of processes, and a single structure for main memory (not a pointer)
*/
int tp, ps, r, X, min, max, k;
struct instruction* instructions;
struct process* processes;
struct memory main_memory;

int hex_to_int (char* string) {
	int final = 0;
	int idx = 0;
	while ((string[idx] >= '0' && string[idx] <= '9')  || (string[idx] >= 'A' && string[idx] <= 'F')) {
		final = final * 16;

		if (string[idx] >= '0' && string[idx] <= '9') {
			final = final + (string[idx] - '0');
		} else if (string[idx] >= 'A' && string[idx] <= 'F') {
			final = final + (string[idx] - 'A') + 10;
		} else {
			fprintf(stderr, "we have an issue with conversion\n");
		}
		idx++;
	}
	return final;
}

int get_address(char* addr_str) {
	if (addr_str[0] == '-') {
		return -1;
	}
	char hex_str[5];
	sscanf(addr_str, "0x%s", hex_str);
	return hex_to_int(hex_str);
}

/*
*	If a page replacement is required, LIFO replaces the page which was recently inserted into memory.
*/
void lifo_page_policy() {
	// Create k child processes
	int process_id = k;
	

	if (process_id == k) {
		/*
		*	Parent process reads the instructions and sends it to the children
		*/
		struct instruction* cur_instruction = instructions;
		
		// Loop until there is no more instructions
		while (cur_instruction != NULL) {

			// If the address of the current instruction is -1, the process is finished
			if (cur_instruction->addr != -1) {
				// check if the page containing the address is in main memory
				int page_number = cur_instruction->addr / ps;

				// search if the page number is in main memory
				struct frame* cur_frame = main_memory.memory_head;
				
				int contains = FALSE;
				while (cur_frame != NULL) {
					if (cur_frame->page_number == page_number) {
						contains = TRUE;
					}
					cur_frame = cur_frame->next_frame;
				}

				// if the page number is not contained in main memory, replace the frame
				if (!contains) {
					// increment number of page fault for process
					for (int i = 0; i < k; i++) {
						if (processes[i].pid == cur_instruction->pid) {
							processes[i].number_of_page_faults += 1;
						}
					}

					// If the number of pages in main memory is less than the number of pages it can hold,
					// simply add it to the front of the list.
					if (main_memory.size < main_memory.max_size) {
						struct frame* new_frame = (struct frame*)malloc(sizeof(struct frame));
						new_frame->page_number = page_number;
						new_frame->previous_frame = NULL;
						new_frame->next_frame = main_memory.memory_head;
						if (main_memory.memory_head == NULL) {
							main_memory.memory_head = new_frame;
							main_memory.memory_head->previous_frame = NULL;
							main_memory.memory_head->next_frame = NULL;
						} else {
							main_memory.memory_head->previous_frame = new_frame;
							main_memory.memory_head = new_frame;
							main_memory.size += 1;
						}
					} else {
						// since only the last page will ever be changed
						// simply change the number in the frame
						main_memory.memory_head->page_number = page_number;
					}
				}
			}


			// FOR DEBUGGING PURPOSES
			if (DEBUG_MODE) {
				// output the frame stack
				struct frame* dbg = main_memory.memory_head;
				printf("stack: ");
				while (dbg != NULL) {
					printf("%d ", dbg->page_number);
					dbg = dbg->next_frame;
				}
				printf("\n");
			}

			// Get to the next instruction
			cur_instruction = cur_instruction->next_instruction;
			
		}
	} else {
		// process waits for 

		exit(EXIT_SUCCESS);
	}

	// Output the results
	printf("----------RESULTS OF LIFO PAGING POLICY----------\n\n");
	
	// print out the number of page faults caused by processes
	int total = 0;
	for (int i = 0; i < k; i++) {
		printf("Page faults caused by process %d: %d\n", processes[i].pid, processes[i].number_of_page_faults);
		total += processes[i].number_of_page_faults;
	}
	printf("Total number of page faults: %d\n", total);
	printf("-------------------------------------------------\n\n");
	
}

/*
*	If a page replacement is required, MRU replaces the page which was most recently used.
*/
void mru_page_policy() {
	// Create k child processes
	int process_id = k;
	

	if (process_id == k) {
		/*
		*	Parent process reads the instructions and sends it to the children
		*/
		struct instruction* cur_instruction = instructions;

		int mru_page_number = -1;
		struct frame* mru_frame = NULL;
		
		// Loop until there is no more instructions
		while (cur_instruction != NULL) {
			// If the address of the current instruction is -1, the process is finished
			if (cur_instruction->addr != -1) {
				// check if the page containing the address is in main memory
				int page_number = cur_instruction->addr / ps;

				// search if the page number is in main memory
				struct frame* cur_frame = main_memory.memory_head;
				
				int contains = FALSE;
				while (cur_frame != NULL && !contains) {
					if (cur_frame->page_number == page_number) {
						contains = TRUE;
						mru_frame = cur_frame;
						mru_page_number = page_number;
					}
					cur_frame = cur_frame->next_frame;
				}

				// if the page number is not contained in main memory, replace most recently used frame
				// which is the last page number which was accessed
				if (!contains) {
					// increment number of page fault for process
					for (int i = 0; i < k; i++) {
						if (processes[i].pid == cur_instruction->pid) {
							processes[i].number_of_page_faults += 1;
						}
					}

					// If the number of pages in main memory is less than the number of pages it can hold,
					// simply add it to the front of the list.
					if (main_memory.size < main_memory.max_size) {
						struct frame* new_frame = (struct frame*)malloc(sizeof(struct frame));
						new_frame->page_number = page_number;
						new_frame->previous_frame = NULL;
						new_frame->next_frame = main_memory.memory_head;
						if (main_memory.memory_head == NULL) {
							main_memory.memory_head = new_frame;
							main_memory.memory_head->previous_frame = NULL;
							main_memory.memory_head->next_frame = NULL;
						} else {
							main_memory.memory_head->previous_frame = new_frame;
							main_memory.memory_head = new_frame;
							main_memory.size += 1;
						}
						mru_frame = main_memory.memory_head;
					} else {
						// look for the page which was most recently used and replace it with the current page

						mru_frame->page_number = page_number;
					}

					// set mru page number and frame
					mru_page_number = page_number;
				}
			}

			
			// FOR DEBUGGING PURPOSES
			if (DEBUG_MODE) {
				// print the most recently used page number
				if (cur_instruction->addr != -1)
					printf("mru page number: %d\t|", mru_page_number);

				// output the frame stack
				struct frame* dbg = main_memory.memory_head;
				printf("stack: ");
				while (dbg != NULL) {
					printf("%d ", dbg->page_number);
					dbg = dbg->next_frame;
				}
				printf("\n");
			}
			


			// Get to the next instruction
			cur_instruction = cur_instruction->next_instruction;
		}
	} else {
		// process waits for 

		exit(EXIT_SUCCESS);
	}

	// Output the results
	printf("----------RESULTS OF MRU PAGING POLICY----------\n\n");
	
	// print out the number of page faults caused by processes
	int total = 0;
	for (int i = 0; i < k; i++) {
		printf("Page faults caused by process %d: %d\n", processes[i].pid, processes[i].number_of_page_faults);
		total += processes[i].number_of_page_faults;
	}
	printf("Total number of page faults: %d\n", total);
	printf("-------------------------------------------------\n\n");


}

/*
*	If a page replacement is required, LRU-X looks for the page whose xth recent access has been the earliest. If there are pages which have not been accessed x times,
*	go for the page which has been accessed the least frequently. If there are multiple pages which have not been accessed at least x times but instead y < x times,
*	choose the page which has been least recently used to be replaced.
*/
void lru_x_page_policy() {
	struct instruction* cur_instruction = instructions;

	// for each frame in main memory, associate it with a "time"
	int gtime = 0;
	
	// Loop until there is no more instructions
	while (cur_instruction != NULL) {
		// If the address of the current instruction is -1, the process is finished
		if (cur_instruction->addr != -1) {
			// Check if the page containing the address is in main memory
			int page_number = cur_instruction->addr / ps;

			// search if the page number is in main memory
			struct frame* cur_frame = main_memory.memory_head;
			
			int contains = FALSE;
			int smallest_frequency = INT_MAX;
			int earliest_time = INT_MAX;
			while (cur_frame != NULL && !contains) {
				if (cur_frame->page_number == page_number) {
					contains = TRUE;
					cur_frame->frequency += 1;
					cur_frame->time = gtime;
				}
				if (cur_frame->frequency < smallest_frequency) {
					smallest_frequency = cur_frame->frequency;
				}
				if (cur_frame->time < earliest_time) {
					earliest_time = cur_frame->time;
				}
				cur_frame = cur_frame->next_frame;
			}

			// If the page number is not contained in main memory, replace frame which has not appeared for at least k times, or
			// if all frames have been around for k times, replace the one which has not been around for the longest.
			if (!contains) {
				// increment number of page fault for process
				for (int i = 0; i < k; i++) {
					if (processes[i].pid == cur_instruction->pid) {
						processes[i].number_of_page_faults += 1;
					}
				}

				// If the number of pages in main memory is less than the number of pages it can hold,
				// simply add it to the front of the list.
				if (main_memory.size < main_memory.max_size) {
					struct frame* new_frame = (struct frame*)malloc(sizeof(struct frame));
					new_frame->page_number = page_number;
					new_frame->previous_frame = NULL;
					new_frame->next_frame = main_memory.memory_head;
					new_frame->frequency = 1;	// the frequency is 1 since the page is already recieved when requested for
					new_frame->time = gtime;
					if (main_memory.memory_head == NULL) {
						main_memory.memory_head = new_frame;
						main_memory.memory_head->previous_frame = NULL;
						main_memory.memory_head->next_frame = NULL;
					} else {
						main_memory.memory_head->previous_frame = new_frame;
						main_memory.memory_head = new_frame;
					}
					main_memory.size += 1;
				} else {
					// look if there is any pages which have frequency less than X
					if (smallest_frequency < X) {
						// If there is a frame with a frequency less than X, then replace that frame.
						// If there are multiple frames with the lowest frequency, replace the one which appears first (the standard LRU)
						struct frame* lru_x_frame = main_memory.memory_head;
						cur_frame = main_memory.memory_head;
						
						while (cur_frame != NULL) {
							if (cur_frame->frequency < lru_x_frame->frequency) {
								lru_x_frame = cur_frame;
							} else if (cur_frame->frequency == lru_x_frame->frequency) {
								if (cur_frame->time < lru_x_frame->time)
								lru_x_frame = cur_frame;
							}
							cur_frame = cur_frame->next_frame;
						}

						// replace the LRU_X frame page number
						lru_x_frame->page_number = page_number;
						lru_x_frame->frequency = 1;
						lru_x_frame->time = gtime;
					} else {
						// look for the frame with the earliest time
						struct frame* lru_x_frame = NULL;
						cur_frame = main_memory.memory_head;
						
						int found_lru_x = FALSE;
						while (cur_frame != NULL && !found_lru_x	) {
							if (cur_frame->time == earliest_time) {
								lru_x_frame = cur_frame;
								found_lru_x = TRUE;
							}
							cur_frame = cur_frame->next_frame;
						}

						// replace the LRU_X frame page number
						lru_x_frame->page_number = page_number;
						lru_x_frame->frequency = 1;
						lru_x_frame->time = gtime;
					}
				}

			}

			// increase the "global" time
			gtime += 1;
		}


		// FOR DEBUGGING PURPOSES
		if (DEBUG_MODE) {
			// OUTPUT FOR DEBUGGING
			if (cur_instruction->addr != -1)
				printf("instruction: %d %d\t|", cur_instruction->pid, cur_instruction->addr / ps);
			else
				printf("instruction: %d -1\t|", cur_instruction->pid);
			// output the frame stack
			struct frame* dbg = main_memory.memory_head;
			printf("stack: ");
			while (dbg != NULL) {
				printf("%d(%d:%d) ", dbg->page_number, dbg->time, dbg->frequency);
				dbg = dbg->next_frame;
			}
			printf("\n");
		}		

		// Get to the next instruction
		cur_instruction = cur_instruction->next_instruction;
	}

	// Output the results
	printf("----------RESULTS OF LRU-X PAGING POLICY----------\n\n");
	
	// print out the number of page faults caused by processes
	int total = 0;
	for (int i = 0; i < k; i++) {
		printf("Page faults caused by process %d: %d\n", processes[i].pid, processes[i].number_of_page_faults);
		total += processes[i].number_of_page_faults;
	}
	printf("Total number of page faults: %d\n", total);
	printf("-------------------------------------------------\n\n");
}

/*
*	If a page replacement is required, LFU looks for the page which has been accessed the least frequently (relatively speaking).
*	If a tie occurs, break it by using the one which has been LRU.
*/
void lfu_page_policy() {
	struct instruction* cur_instruction = instructions;

	// for each frame in main memory, associate it with a "time"
	int gtime = 0;

	// Loop until there is no more instructions
	while (cur_instruction != NULL) {
		// If the address of the current instruction is -1, the process is finished
		if (cur_instruction->addr != -1) {
			// check if the page containing the address is in main memory
			int page_number = cur_instruction->addr / ps;

			// search if the page number is in main memory
			struct frame* cur_frame = main_memory.memory_head;
			
			int contains = FALSE;
			while (cur_frame != NULL && !contains) {
				if (cur_frame->page_number == page_number) {
					contains = TRUE;
					cur_frame->frequency += 1;
					cur_frame->time = gtime;
				}
				cur_frame = cur_frame->next_frame;
			}

			// if the page number is not contained in main memory, replace the least frequently accessed frame
			// which is the last page number which was accessed
			if (!contains) {
				// increment number of page fault for process
				for (int i = 0; i < k; i++) {
					if (processes[i].pid == cur_instruction->pid) {
						processes[i].number_of_page_faults += 1;
					}
				}

				// If the number of pages in main memory is less than the number of pages it can hold,
				// simply add it to the front of the list.
				if (main_memory.size < main_memory.max_size) {
					struct frame* new_frame = (struct frame*)malloc(sizeof(struct frame));
					new_frame->page_number = page_number;
					new_frame->previous_frame = NULL;
					new_frame->next_frame = main_memory.memory_head;
					new_frame->frequency = 1;
					new_frame->time = gtime;
					if (main_memory.memory_head == NULL) {
						main_memory.memory_head = new_frame;
						main_memory.memory_head->previous_frame = NULL;
						main_memory.memory_head->next_frame = NULL;
					} else {
						main_memory.memory_head->previous_frame = new_frame;
						main_memory.memory_head = new_frame;
					}
					main_memory.size += 1;
				} else {
					// Look for the frame with the lowest frequency
					// If it's a tie, the frame that was used least recently is chosen
					struct frame* lfu_frame = main_memory.memory_head;
					cur_frame = main_memory.memory_head;
					
					while (cur_frame != NULL) {
						if (cur_frame->frequency < lfu_frame->frequency) {
							lfu_frame = cur_frame;
						} else if (lfu_frame->frequency == cur_frame->frequency) {
							if (cur_frame->time < lfu_frame->time) {
								lfu_frame = cur_frame;
							}
						}
						cur_frame = cur_frame->next_frame;
					}

					// replace it with the new page number
					lfu_frame->page_number = page_number;
					lfu_frame->frequency = 1;
					lfu_frame->time = gtime;

				}

			}

			// increase the "global" time
			gtime += 1;
		}

		// FOR DEBUGGING PURPOSES
		if (DEBUG_MODE) {
			// OUTPUT FOR DEBUGGING
			struct frame* dbg = main_memory.memory_head;
			printf("stack: ");
			while (dbg != NULL) {
				printf("%d(%d) ", dbg->page_number, dbg->frequency);
				dbg = dbg->next_frame;
			}
			printf("\n");
		}

		// Get to the next instruction
		cur_instruction = cur_instruction->next_instruction;
	}

	// Output the results
	printf("----------RESULTS OF LFU PAGING POLICY----------\n\n");
	
	// print out the number of page faults caused by processes
	int total = 0;
	for (int i = 0; i < k; i++) {
		printf("Page faults caused by process %d: %d\n", processes[i].pid, processes[i].number_of_page_faults);
		total += processes[i].number_of_page_faults;
	}
	printf("Total number of page faults: %d\n", total);
	printf("-------------------------------------------------\n\n");

}


/*
*	If a page replacement is required, OPT-X looks X times into the future to see which pages appear the latest into the future.
*	So if the size of main memory was 3 (5 3 1), and the page requested for was 2, if in the next X instructions, 1 was the last page
*	to appear again, 2 would replace 1.
*/
void opt_lookahead_x_page_policy() {
	struct instruction* cur_instruction = instructions;
	
	// Loop until there is no more instructions
	while (cur_instruction != NULL) {
		// If the address of the current instruction is -1, the process is finished
		if (cur_instruction->addr != -1) {
			// Check if the page containing the address is in main memory
			int page_number = cur_instruction->addr / ps;

			// search if the page number is in main memory
			struct frame* cur_frame = main_memory.memory_head;
			
			int contains = FALSE;
			while (cur_frame != NULL && !contains) {
				if (cur_frame->page_number == page_number) {
					contains = TRUE;
				}
				cur_frame = cur_frame->next_frame;
			}

			if (!contains) {
				// increment number of page fault for process
				for (int i = 0; i < k; i++) {
					if (processes[i].pid == cur_instruction->pid) {
						processes[i].number_of_page_faults += 1;
					}
				}

				// If the number of pages in main memory is less than the number of pages it can hold,
				// simply add it to the front of the list.
				if (main_memory.size < main_memory.max_size) {
					struct frame* new_frame = (struct frame*)malloc(sizeof(struct frame));
					new_frame->page_number = page_number;
					new_frame->previous_frame = NULL;
					new_frame->next_frame = main_memory.memory_head;
					new_frame->frequency = 0;
					if (main_memory.memory_head == NULL) {
						main_memory.memory_head = new_frame;
						main_memory.memory_head->previous_frame = NULL;
						main_memory.memory_head->next_frame = NULL;
					} else {
						main_memory.memory_head->previous_frame = new_frame;
						main_memory.memory_head = new_frame;
					}
					main_memory.size += 1;
				} else {
					// Replace the frame in main memory which appears the last in the future X frames.

					// Set the time for all the frames in which it appears in the future as INT_MAX (meaning it will not appear)
					cur_frame = main_memory.memory_head;
					while (cur_frame != NULL) {
						cur_frame->time = INT_MAX;
						cur_frame = cur_frame->next_frame;
					}

					// Go through X instructions and find the frame in memory which is seen last
					struct instruction* future_instr = cur_instruction->next_instruction;
					for (int j = 0; j < X && future_instr != NULL; j++) {
						if (future_instr->addr != -1) {
							// if the page the address points to is in main memory, update time accordingly
							cur_frame = main_memory.memory_head;
							int addr_in_mem = FALSE;
							while (cur_frame != NULL && !addr_in_mem) {
								if (cur_frame->page_number == (future_instr->addr / ps) &&
									(cur_frame->time == INT_MAX)) {
									addr_in_mem = TRUE;
									cur_frame->time = j;
								}
								cur_frame = cur_frame->next_frame;
							}
						} else {
							j--;
						}
						future_instr = future_instr->next_instruction;
					}

					// Find the longest time
					struct frame* opt_x_frame = NULL;
					int longest_time = -1;
					cur_frame = main_memory.memory_head;

					while (cur_frame != NULL) {
						if (cur_frame->time > longest_time) {
							longest_time = cur_frame->time;
							opt_x_frame = cur_frame;
						}
						cur_frame = cur_frame->next_frame;
					}

					// change the frame of the optimal x frame
					opt_x_frame->page_number = page_number;
				}

			}
		}

		// FOR DEBUGGING PURPOSES
		if (DEBUG_MODE) {
			if (cur_instruction->addr != -1)
				printf("current_page %d\t|", cur_instruction->addr / ps);
			
			// output the frame stack
			struct frame* dbg = main_memory.memory_head;
			printf("stack: ");
			while (dbg != NULL) {
				printf("%d ", dbg->page_number);
				dbg = dbg->next_frame;
			}
			printf("\n");
		}
		


		// Get to the next instruction
		cur_instruction = cur_instruction->next_instruction;
	}



	// Output the results
	printf("----------RESULTS OF OPT-X PAGING POLICY----------\n\n");
	
	// print out the number of page faults caused by processes
	int total = 0;
	for (int i = 0; i < k; i++) {
		printf("Page faults caused by process %d: %d\n", processes[i].pid, processes[i].number_of_page_faults);
		total += processes[i].number_of_page_faults;
	}
	printf("Total number of page faults: %d\n", total);
	printf("-------------------------------------------------\n\n");
}

/*
*	In the working set page policy, if the process is currently running, update the working set by looking back in time for X cases.
*	If a page needs to be replaced, simply stop the execution of processes which have the smallest working set size. The rationale is that
*	processes which have the smallest working set has not been running recently and so can be ignored to "finish" processes which have been
*	working. 
*
*	Since this is the last page replacement policy, instructions will be "thrown" out after it is read.
*/
void ws_page_policy() {
	struct instruction* cur_instruction = instructions;
	
	// define a "global" time
	int gtime = 0;

	// Loop until there are no more instructions
	while (cur_instruction != NULL) {
		// Check if a process is currently running, if it is not read, and if
		int is_currently_running = FALSE;
		int pid;
		for (pid = 0; pid < k; pid++) {
			if (processes[pid].currently_running) {
				is_currently_running = TRUE;
				break;
			}
		}

		
		if (cur_instruction->addr != -1) {
			// If the process is currently running, and the instruction is for an address, get the page number
			int page_number = cur_instruction->addr / ps;

			// Next, check if the page number is in the working set of the process
			struct frame* wspi = processes[pid].working_set;

			int in_working_set = FALSE;
			while (wspi != NULL) {
				if (wspi->page_number == page_number) {
					in_working_set = TRUE;
					wspi->time = gtime;		// update the time the page was in working set
				}
			}

			// if the page number is in the working set of the process
		} else {
			processes[pid].currently_running = FALSE;
		}
	}


	// Loop until there is no more instructions
	while (cur_instruction != NULL) {
		// If the address of the current instruction is -1, the process is finished
		if (cur_instruction->addr != -1) {
			// Check if the page containing the address is in main memory
			int page_number = cur_instruction->addr / ps;

			// check if instruction pid is valid
			if (cur_instruction->pid is a part of currently running processes) {




				//
			} else {

			}

			// search if the page number is in main memory
			struct frame* cur_frame = main_memory.memory_head;
			
			int contains = FALSE;
			while (cur_frame != NULL && !contains) {
				if (cur_frame->page_number == page_number) {
					contains = TRUE;
				}
				cur_frame = cur_frame->next_frame;
			}

			// check if thrashing occurs


			if (!contains) {
				// increment number of page fault for process
				for (int i = 0; i < k; i++) {
					if (processes[i].pid == cur_instruction->pid) {
						processes[i].number_of_page_faults += 1;
					}
				}

				// If the number of pages in main memory is less than the number of pages it can hold,
				// simply add it to the front of the list.
				if (main_memory.size < main_memory.max_size) {
					struct frame* new_frame = (struct frame*)malloc(sizeof(struct frame));
					new_frame->page_number = page_number;
					new_frame->previous_frame = NULL;
					new_frame->next_frame = main_memory.memory_head;
					new_frame->frequency = 0;
					if (main_memory.memory_head == NULL) {
						main_memory.memory_head = new_frame;
						main_memory.memory_head->previous_frame = NULL;
						main_memory.memory_head->next_frame = NULL;
					} else {
						main_memory.memory_head->previous_frame = new_frame;
						main_memory.memory_head = new_frame;
					}
					main_memory.size += 1;
				} else {
					// Replace the frame in main memory which appears the last in the future X frames.

					// Set the time for all the frames in which it appears in the future as INT_MAX (meaning it will not appear)
					cur_frame = main_memory.memory_head;
					while (cur_frame != NULL) {
						cur_frame->time = INT_MAX;
						cur_frame = cur_frame->next_frame;
					}

					// Go through X instructions and find the frame in memory which is seen last
					struct instruction* future_instr = cur_instruction->next_instruction;
					for (int j = 0; j < X && future_instr != NULL; j++) {
						if (future_instr->addr != -1) {
							// if the page the address points to is in main memory, update time accordingly
							cur_frame = main_memory.memory_head;
							int addr_in_mem = FALSE;
							while (cur_frame != NULL && !addr_in_mem) {
								if (cur_frame->page_number == (future_instr->addr / ps) &&
									(cur_frame->time == INT_MAX)) {
									addr_in_mem = TRUE;
									cur_frame->time = j;
								}
								cur_frame = cur_frame->next_frame;
							}
						} else {
							j--;
						}
						future_instr = future_instr->next_instruction;
					}

					// Find the longest time
					struct frame* opt_x_frame = NULL;
					int longest_time = -1;
					cur_frame = main_memory.memory_head;

					while (cur_frame != NULL) {
						if (cur_frame->time > longest_time) {
							longest_time = cur_frame->time;
							opt_x_frame = cur_frame;
						}
						cur_frame = cur_frame->next_frame;
					}

					// change the frame of the optimal x frame
					opt_x_frame->page_number = page_number;
				}

			}
		}

		// FOR DEBUGGING PURPOSES
		if (DEBUG_MODE) {
			if (cur_instruction->addr != -1)
				printf("current_page %d\t|", cur_instruction->addr / ps);
			
			// output the frame stack
			struct frame* dbg = main_memory.memory_head;
			printf("stack: ");
			while (dbg != NULL) {
				printf("%d ", dbg->page_number);
				dbg = dbg->next_frame;
			}
			printf("\n");
		}
		


		// Get to the next instruction
		cur_instruction = cur_instruction->next_instruction;
	}



	// Output the results
	printf("----------RESULTS OF WS PAGING POLICY----------\n\n");
	
	// print out the number of page faults caused by processes
	int total = 0;
	for (int i = 0; i < k; i++) {
		printf("Page faults caused by process %d: %d\n", processes[i].pid, processes[i].number_of_page_faults);
		total += processes[i].number_of_page_faults;
	}
	printf("Total number of page faults: %d\n", total);
	printf("-------------------------------------------------\n\n");
}

/*
*	Clear all the frames inside main memory, and reset the information inside processes to its default value
*/
void clear_for_next_page_policy() {
	// set the number of page faults back to 0
	for (int i = 0; i < k; i++) {
		processes[i].number_of_page_faults = 0;
	}

	// destroy working set
	for (int i = 0; i < k; i++) {
		// clear out working set of process i
		struct frame* ws = processes[i].working_set;
		while (ws != NULL) {
			processes[i].working_set = (processes[i].working_set)->next_frame;
			free(ws);
			ws = processes[i].working_set;
		}
	}

	// clear out main memory
	struct frame* node = main_memory.memory_head;
	while (node != NULL) {
		main_memory.memory_head = main_memory.memory_head->next_frame;
		free(node);
		node = main_memory.memory_head;
	}
	main_memory.size = 0;
}

/*
*	This program simulates how page replacement policies work to get the required pages needed to read an address.
*	Given 
*		1. the number of frames (which is associated with physical part of memory) in main memory
*		2. the size of a page, 
*		3. the number of pages per process (which seem to be the same as the number of pages a process has)
*		4. the lookahead window size,
*		5. the smallest amount of free frames allowed,
*		6. the maximum amount of free frames allowed, and 
*		7. the total number of processes
*	and a list of requests from processes for information from the address in virtual memory they are requesting for, find 
*	for each paging policy, the number of page faults caused by each process requesting for an address not stored in main memory,
*	and the number of page faults in total.
*	For the working set page policy, also output the maximum and minimum size of the working set.
*/
int main(int argc, char** argv) {
	/*
	*	The single argument given is the input file. The input format is specified in README.
	*/
	FILE* input = fopen(argv[1], "r");

	/*
	*   If the files are not provided, exit the program
	*/
	if (input == NULL) {
		printf("The file is null\n");
		exit(EXIT_FAILURE);
	}

	/*
	*	The lines will not exceed 1000 characters, so allocate 1000 characters as a buffer.
	*	Read the first seven integers, and put them into the relevant GLOBAL variables
	*
	*	It seems unlikely that min and max will be used, but it is read anyways
	*/

	fscanf(input, "%d", &tp);
	fscanf(input, "%d", &ps);
	fscanf(input, "%d", &r);
	fscanf(input, "%d", &X);
	fscanf(input, "%d", &min);
	fscanf(input, "%d", &max);
	fscanf(input, "%d", &k);

	/*
	*	Initialize main memory
	*/
	main_memory.max_size = tp;
	main_memory.size = 0;
	main_memory.memory_head = NULL;

	/*
	*   Initialize k processes.
	*
	*/
	processes = (struct process*)malloc(sizeof(struct process) * k);
	int proc_id, size;
	for (int i = 0; i < k; i++) {
		fscanf(input, "%d %d", &proc_id, &size);
		processes[i].pid = proc_id;
		processes[i].number_of_pages = size;
		processes[i].number_of_page_faults = 0;

		// These two are for the working set page policy
		processes[i].working_set = NULL;
		processes[i].currently_running = TRUE;
	}

	/*
	*	Read instructions and store it in a linked list
	*/
	instructions = NULL;
	int cpid, caddr;
	char caddr_str[5];
		
	while (fscanf(input, "%d %s", &cpid, caddr_str) != EOF) {
		caddr = get_address(caddr_str);
		add_instruction(&instructions, cpid, caddr);
		memset(caddr_str, 0, 5);
	}
	fclose(input);

	// check if the instructions are actually read
	struct instruction* temp = instructions;
	while (temp != NULL) {
		if (temp->addr != -1)
			printf("%d %d\n", temp->pid, temp->addr / ps);
		else
			printf("%d -1\n", temp->pid);

		temp = temp->next_instruction;
	}

	/*
	*	After reading the input, test each of the page replacement algorithms: LIFO (last-in-first-out), MRU (most recently used), LRU-X (least-recently-used),
	*	LFU (least-frequently-used), OPT-lookahead-X, and WS (working set)
	*
	*	The working set algorithm is not encapsulated in a method since forks of main  
	*/
	lifo_page_policy();
	clear_for_next_page_policy();

	mru_page_policy();
	clear_for_next_page_policy();

	lru_x_page_policy();
	clear_for_next_page_policy();

	lfu_page_policy();
	clear_for_next_page_policy();

	opt_lookahead_x_page_policy();
	clear_for_next_page_policy();

	//ws_page_policy();
	clear_for_next_page_policy();

	/*
	*	Free list of instructions and array of processes.
	*/
	destroy_instruction_list(instructions);
	free(processes);

    return 0;
}