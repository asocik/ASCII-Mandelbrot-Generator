/*----------------------------------------------------------------------------
 *
 * Author: Adam Socik
 * File: mandelDisplay.cpp
 * Created: February-March 2014
 *
 ----------------------------------------------------------------------------*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>

using namespace std;

// Message struct used in message queues
typedef struct
{
    long type;
    int child;
    char fileName[80];
} message;

int number_of_images_calculated = 0;

/*----------------------------------------------------------------------------
 Signal handler for SIGUSR1
 ----------------------------------------------------------------------------*/
void signalHandler(int sig)
{
    exit(number_of_images_calculated);
}

/*----------------------------------------------------------------------------
 Main calculates the ASCII art for the image
 ----------------------------------------------------------------------------*/
int main(int argc, const char * argv[])
{
    // Set up shared memory
    int shmid = atoi(argv[1]);
    int *data = (int *)shmat(shmid, 0, 0);

    char pipeInput[200];
    char pipeOutput[200];

    // Register signal SIGUSR1
    if (signal(SIGUSR1, signalHandler) == SIG_ERR)
    {
		cerr << "Error: Signal Handler in calc\n";
		exit(-1);
    }

    while (true)
    {
        // Initialize to all null
        for (int i=0; i<200; i++)
            pipeInput[i] = '\0';

        cin.getline(pipeInput, 200);    // Read entire line from pipe
        strncpy(pipeOutput, pipeInput, 200);

        //cerr << "Calc pipe input: " << pipeInput << endl;

        // Store data in appropriate variables
        float xMin = atof(strtok(pipeInput, " "));
        float xMax = atof(strtok(NULL, " "));
        float yMin = atof(strtok(NULL, " "));
        float yMax = atof(strtok(NULL, " "));
        int nRows = atoi(strtok(NULL, " "));
        int nCols = atoi(strtok(NULL, " "));
        int maxIters = atoi(strtok(NULL, " "));

        /*
         cerr << "xMin in calc: " << xMin << endl;
         cerr << "xMax in calc: " << xMax << endl;
         cerr << "yMin in calc: " << yMin << endl;
         cerr << "yMax in calc: " << yMax << endl;
         cerr << "nRows in calc: " << nRows << endl;
         cerr << "nCols in calc: " << nCols << endl;
         cerr << "maxIters in calc: " << maxIters << endl;
         */

        //----------------------------------------------------------------
        // Mandelbrot Algorithm
        //----------------------------------------------------------------
        float deltaX = (xMax-xMin) / (nCols-1);
        float deltaY = (yMax-yMin) / (nRows-1);
        float cy, cx, zx, zy, zx_next, zy_next;
        int r, c, n;

        for (r=0; r<nRows-1; r++)
        {
            cy = yMin +r * deltaY;
            for (c=0; c<nCols-1; c++)
            {
                cx = xMin + c * deltaX;
                zx = zy = 0.0;
                for(n = 0; n < maxIters; n++)
                {
                    if( zx * zx + zy * zy >= 4.0 )
                        break;

                    zx_next = zx * zx - zy * zy + cx;
                    zy_next = 2.0 * zx * zy + cy;
                    zx = zx_next;
                    zy = zy_next;
                } // End for n

                if (n >= maxIters)
                    *(data + r * nCols + c) = -1;
                else
                    *(data + r * nCols + c) = n;
            } // End for c
        } // End for r

        // Pipe output to the next child
        pipeOutput[strlen(pipeOutput)] = '\n';  // Needed to accept input
        cout << pipeOutput;
        fflush(stdout);

        // Send message to parent that child has finished
        int msgid = atoi(argv[2]);
        message m;
        msgget(msgid, IPC_CREAT | 0600);
        m.type = 3;
        m.child = 1;
        strncpy(m.fileName, "Calc is done", 80);
        if (msgsnd(msgid, &m, (sizeof(m)-sizeof(long)), 0) == -1)
        {
            cerr << "mandelCalc message to parent failed to send\n";
            exit(-2);
        }

        number_of_images_calculated++;
    }
    return 0;
}
