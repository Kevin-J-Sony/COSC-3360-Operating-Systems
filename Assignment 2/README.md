To compile this code, run:

gcc -g deadlock_avoidance.c -o main

To execute this code, run:

./main <process_file> <word_file>

I would like to use one grace days for this assignment.

For the input, the first two lines are filled by m and n, and the next m lines are the values of the AVAILABLE array. The next n*m lines contain the values of
the MAX matrix. I assume that the allocation matrix is zero initially. Furthermore, for the resource word input, I assume that every word is either all lower case, or has
at an upper case at the beginning. This is because with this assumption, alphabetical order can easily be given by strcmp.

For the output, combining elements in the master string was not taken since it would be more complex. Therefore, for this reason, the master string is outputted alphebetically.
Hopefully, the grader is understanding of this.

The code on the Linux server is the most up-to-date, but I should have also submitted it as a zip file.