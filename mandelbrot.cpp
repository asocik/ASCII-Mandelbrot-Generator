/*----------------------------------------------------------------------------
 *
 * Author: Adam Socik
 * File: mandelDisplay.cpp
 * Created: February-March 2014
 *
 ----------------------------------------------------------------------------*/
/*
 This program generates an ASCII version of a Mandelbrot image. This is done
 by using pipes, shared memory, and message queues and forking work off to
 child processes.
 */
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

#define READ_END 0
#define WRITE_END 1

using namespace std;

// Message struct used in message queues
typedef struct
{
    long type;
    int child;
    char fileName[80];
} message;

pid_t mandelCalcChildID = -1;
pid_t mandelDisplayID = -1;
int calcStatus;
int displayStatus;

/*----------------------------------------------------------------------------
 Signal Handler for SIGCHLD - reports exit status of children
 ----------------------------------------------------------------------------*/
void signalHandler(int sig)
{
    waitpid(mandelCalcChildID, &calcStatus, 0);
    waitpid(mandelDisplayID, &displayStatus, 0);
    if (calcStatus == -1 || displayStatus == -1)
    {
        cerr << "Error: waitpid()\n";
        exit(EXIT_FAILURE);
    }

    // Inspect the exit status
    if (WIFEXITED(calcStatus))
        cerr << "Child mandelCalc terminated normally\n";
    else if (WIFSIGNALED(calcStatus))
        cerr << "Child mandelCalc terminated with status: " <<  WTERMSIG(calcStatus) << endl;
    else
        cout << "The cause of death of mandelCalc is undetermined at this time.\n";

    if (WIFEXITED(displayStatus))
        cerr << "Child mandelDisplay terminated normally\n";
    else if (WIFSIGNALED(displayStatus))
        cerr << "Child mandelDisplay terminated with status: " <<  WTERMSIG(displayStatus) << endl;
    else
        cout << "The cause of death of mandelDisplay is undetermined at this time.\n";
}

/*----------------------------------------------------------------------------
 Sets up message queues, shared memory, pipes, and forks to children. Main also
 handles user input.
 ----------------------------------------------------------------------------*/
int main(int argc, const char * argv[])
{
    cout << "\nName: Adam Socik\tACCC ID: asocik2\n\n";

    //----------------------------------------------------------------
    // Register signal handler for SIGCHILD and SIGUSR1
    //----------------------------------------------------------------
    if(signal(SIGCHLD, signalHandler) == SIG_ERR)
    {
        cerr << "Error: Signal Handler in parent\n";
        exit(EXIT_FAILURE);
    }

    //----------------------------------------------------------------
    // Create the message queues
    //----------------------------------------------------------------
    int msgid1, msgid2;
    char msgid1String[15], msgid2String[15];

    if ((msgid1 = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) < 0)
    {
        cerr << "msgid1 error\n";
        exit(EXIT_FAILURE);
    }

    if ((msgid2 = msgget(IPC_PRIVATE, IPC_CREAT | 0600))< 0)
    {
        cerr << "msgid2 error\n";
        exit(EXIT_FAILURE);
    }

    // Convert ids to strings so that they can be passed as arguments in exec
    sprintf(msgid1String, "%d", msgid1);
    sprintf(msgid2String, "%d", msgid2);

    //----------------------------------------------------------------
    // Create the shared memory
    //----------------------------------------------------------------
    int shmid;
    char shmidString[15];

    if ((shmid = shmget(IPC_PRIVATE, 20000, IPC_CREAT | 0600)) == -1)
    {
        cerr << "Shared memory error\n";
        exit(EXIT_FAILURE);
    }

    sprintf(shmidString, "%d", shmid);  // Convert shared memory id to string

    //----------------------------------------------------------------
    // Create the pipes
    //----------------------------------------------------------------
    int calcPipe[2];
    int displayPipe[2];

    if (pipe(calcPipe) == -1)
    {
        cerr << "calcPipe error\n";
        exit(EXIT_FAILURE);
    }

    if (pipe(displayPipe) == -1)
    {
        cerr << "displayPipe error\n";
        exit(EXIT_FAILURE);
    }

    //----------------------------------------------------------------
    // Fork first child - mandelCalc
    //----------------------------------------------------------------
    mandelCalcChildID = fork();

    if(mandelCalcChildID < 0)
    {
        cerr <<  "Fork for mandelCalcChild failed!\n";
        exit(EXIT_FAILURE);
    }
    else if (mandelCalcChildID == 0)    // Code executed by child
    {

        dup2(calcPipe[READ_END], 0);        // Redirect standard input to read end of calcPipe
        dup2(displayPipe[WRITE_END], 1);    // Redirect standard output to write end of displayPipe

        // Close the unneeded part of the pipes
        close(calcPipe[WRITE_END]);
        close(calcPipe[READ_END]);
        close(displayPipe[WRITE_END]);
        close(displayPipe[READ_END]);

        execlp("./mandelCalc", "mandelCalc", shmidString, msgid1String, NULL);

        // Should never get here
        cerr << "Child process execution wrong!\n";
        exit(0);
    }

    //----------------------------------------------------------------
    // Fork second child - mandelDisplay
    //----------------------------------------------------------------
    mandelDisplayID = fork();

    if (mandelDisplayID < 0)
    {
        cerr <<  "Fork for mandelDisplay failed!\n";
        exit(EXIT_FAILURE);
    }
    else if (mandelDisplayID == 0)
    {
        dup2(displayPipe[READ_END], 0);     // Redirect standard input to read end of displayPipe

        // Close the unneeded part of the pipes
        close(calcPipe[WRITE_END]);
        close(calcPipe[READ_END]);
        close(displayPipe[WRITE_END]);
        close(displayPipe[READ_END]);

        execlp("./mandelDisplay", "mandelDisplay", shmidString, msgid1String, msgid2String, NULL);

        // Should never get here
        cerr << "Child process execution wrong!\n";
        exit(EXIT_FAILURE);
    }

    //----------------------------------------------------------------
    // Code only executed by the parent process
    //----------------------------------------------------------------
    dup2(calcPipe[WRITE_END], 1);       // Redirect standard output to write end of calcPipe

    // Close the unneeded part of the pipes
    close(calcPipe[WRITE_END]);
    close(calcPipe[READ_END]);
    close(displayPipe[WRITE_END]);
    close(displayPipe[READ_END]);

    float xMin, xMax, yMin, yMax;
    int nRows, nCols, maxIters, in;
    char filename[80];
    bool done = false;

    // Initialize filename to all null
    for (int i=0; i<80; i++)
        filename[i] = '\0';

    while (true)
    {
        if (!done)
        {
            cerr << "Enter a filename (no spaces) to save data -> ";
            cin  >> filename;
            cin.ignore(256, '\n');  // Ignore any words after a space
            cerr << "Enter a value for xMin -> ";
            cin  >> xMin;
            cerr << "Enter a value for xMax -> ";
            cin  >> xMax;
            cerr << "Enter a value for yMin -> ";
            cin  >> yMin;
            cerr << "Enter a value for yMax -> ";
            cin  >> yMax;
            cerr << "Enter a value for nRows -> ";
            cin  >> nRows;
            cerr << "Enter a value for nCols -> ";
            cin  >> nCols;
            cerr << "Enter a value for maxIters -> ";
            cin  >> maxIters;

            // Convert the output to one string
            char pipeOutput[200];
            for (int i=0; i<200; i++)
                pipeOutput[i] = '\0';

            sprintf(pipeOutput, "%f %f %f %f %d %d %d\n",
                    xMin, xMax, yMin, yMax, nRows, nCols, maxIters);

            // Send all data through the pipe in one string
            cout << pipeOutput;
            fflush(stdout);

            // send message of filename to mandelDisplay
            message m;
            m.type = 1;
            m.child = 0;
            strncpy(m.fileName, filename, 80);

            if (msgsnd(msgid2, &m, (sizeof(m)-sizeof(long)), 0) == -1)
            {
                cerr << "Parent message of filename to mandelDispaly failed to send\n";
                exit(EXIT_FAILURE);
            }

            // Wait for messages from both children to show that they are done
            message m2;
            message m3;
            msgrcv(msgid1, &m3, (sizeof(m3)-sizeof(long)), 3, 0);
            //cerr << "Message: " << m3.fileName << endl;
            msgrcv(msgid1, &m2, (sizeof(m2)-sizeof(long)), 2, 0);
            //cerr << "Message: " << m2.fileName << endl;

            // Initialize filename to all null - clear out old name
            for (int i=0; i<80; i++)
                filename[i] = '\0';
        }
        else
        {
            // User is done so send signal for children to exit
            kill(mandelCalcChildID, SIGUSR1);
            kill(mandelDisplayID, SIGUSR1);
            waitpid(mandelCalcChildID, &calcStatus, 0);
            waitpid(mandelDisplayID, &displayStatus, 0);
            break;
        }

        // Check to see if the user is done entering values
        cerr << "Are you done (1 for yes, 0 for no) -> ";
        cin  >> in;

        if (in == 1)
            done = true;
    } // End while(true)

    // Close message queues and shared memory
    msgctl(msgid1, IPC_RMID, NULL);
    msgctl(msgid2, IPC_RMID, NULL);
    shmctl(shmid, IPC_RMID, NULL);

    cerr << "\nDone\n\n";
    return 0;
}

