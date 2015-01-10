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
#include <fstream>

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
 Main generates the ASCII image using the values from mandelCalc
 ----------------------------------------------------------------------------*/
int main(int argc, const char * argv[])
{
    // Register signal SIGUSR1
    if (signal(SIGUSR1, signalHandler) == SIG_ERR)
    {
		cerr << "Error: Signal Handler in calc\n";
		exit(1);
    }
    char pipeInput[200];

    while (true)
    {
        // Initialize to all null
        for (int i=0; i<200; i++)
            pipeInput[i] = '\0';

        cin.getline(pipeInput, 200);    // Read entire line from pipe

        //cerr << "Display pipe input: " << pipeInput << endl;

        // Store data in appropriate variables
        float xMin = atof(strtok(pipeInput, " "));
        float xMax = atof(strtok(NULL, " "));
        float yMin = atof(strtok(NULL, " "));
        float yMax = atof(strtok(NULL, " "));
        int nRows = atoi(strtok(NULL, " "));
        int nCols = atoi(strtok(NULL, " "));
        int maxIters = atoi(strtok(NULL, " "));

        /*
         cerr << "xMin in display: " << xMin << endl;
         cerr << "xMax in display: " << xMax << endl;
         cerr << "yMin in display: " << yMin << endl;
         cerr << "yMax in display: " << yMax << endl;
         cerr << "nRows in display: " << nRows << endl;
         cerr << "nCols in display: " << nCols << endl;
         cerr << "maxIters in display: " << maxIters << endl;
         */

        //----------------------------------------------------------------
        // Get filename from message queue and set next message
        //----------------------------------------------------------------
        int msgid1 = atoi(argv[2]);
        int msgid2 = atoi(argv[3]);
        message m;
        message m2;

        // Get message of filename from the parent
        msgget(msgid2, IPC_CREAT | 0600);
        msgrcv(msgid2, &m, (sizeof(m)-sizeof(long)), 1, 0);

        //----------------------------------------------------------------
        // Algorithm for Displaying Mandelbrot Values
        //----------------------------------------------------------------
        int shmid = atoi(argv[1]);
        int *data = (int *)shmat(shmid, 0, 0);

        char colors[] = ".-~:+*%O8&?$@#X";

        int r,c,n;

        for (r=nRows-1; r>=0; r--)
        {
            for (c=0; c<nCols-1; c++)
            {
                n = *(data + r * nCols + c); // = data[r][c]
                if( n < 0 )
                    cout << " ";
                else
                    cout << colors[n % 15];
            }
            cout << endl;
        }

        //----------------------------------------------------------------
        // Open a file and copy in data from shared memory
        //----------------------------------------------------------------
        ofstream dataFile (m.fileName);

        // If the file was successfully opened
        if (dataFile.is_open())
        {
            for (r=0; r<nRows-1; r++)
            {
                for (c=0; c<nCols-1; c++)
                {
                    dataFile << *(data + r * nCols + c) << " ";
                } // End for c
                dataFile << endl;
            } // End for r

            dataFile.close();
        }

        // Send message to parent that child has finished
        msgget(msgid1, IPC_CREAT | 0600);
        m2.type = 2;
        m2.child = 2;
        strncpy(m2.fileName, "Display is done", 80);
        if (msgsnd(msgid1, &m2, (sizeof(m2)-sizeof(long)), 0) == -1)
        {
            cerr << "mandelDisplay message to parent failed to send\n";
            exit(1);
        }

        number_of_images_calculated++;
    }

    return 0;
}

