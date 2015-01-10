# ASCII-Mandelbrot-Generator
The ASCII Mandelbrot Generator is, as the name describes, a program that generates an ASCII representation of a Mandelbrot image. This program has an intentionally overcomplicated design that uses several inter-process communication (IPC) methods in C: signals, pipes, shared memory, and message queues.

To run the program just type "make all" on the command line and then "./mandelbrot"
You can type "make clean" to get rid of all three executables
