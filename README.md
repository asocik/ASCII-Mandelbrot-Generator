# ASCII-Mandelbrot-Generator
The ASCII Mandelbrot Generator is, as the name describes, a program that generates an ASCII representation of a Mandelbrot image. This program has an intentionally overcomplicated design that uses several inter-process communication (IPC) methods in C: signals, pipes, shared memory, and message queues.

To run the program just type "make all" on the command line and then "./mandelbrot".
Type "make clean" to get rid of all three executables.

To use sample input files type "./mandelbrot < input1.txt" or "./mandelbrot < input2.txt"

