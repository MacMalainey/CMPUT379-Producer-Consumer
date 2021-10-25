#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <time.h>

#include "tasks.h"

// Templates for the logfile name
#define LOGNAME_TEMPLATE "prodcon.%d.log"
#define LOGNAME_DEFAULT "prodcon.log"

// Command ids
#define SLEEP_CMD 'S'
#define TASK_CMD 'T'
#define NO_CMD 'N'

// Format for how a command should be presented
#define ICMD_FORMAT "%c%d"

/**
 * Command struct
 */
typedef struct icommand {
    /**
     * cmd type
     */
    char cmd;

    /**
     * command arg
     */
    int n;
} icmd_t;

/**
 * Retrieve next command from input
 */
icmd_t next_icmd()
{
    icmd_t command;
    if (scanf(ICMD_FORMAT, &(command.cmd), &(command.n)) == -1)  // Exit on end of input
    {
        command.cmd = NO_CMD;
    }
    return command;
}

/**
 * Main program loop
 * 
 * Handles the retrieval of new commands from input and determining what to do with them
 */
void loop(tman_t* taskman)
{
    icmd_t command;
    while (1)
    {
        // Get command
        command = next_icmd();

        // Handle task command
        if (command.cmd == TASK_CMD)
        {
            tman_publish(taskman, command.n);
        }
        // Handle sleep command
        else if (command.cmd == SLEEP_CMD)
        {
            tman_sleep(taskman, command.n);
        }
        // Handle no command
        else if (command.cmd == NO_CMD)
        {
            tman_close(taskman);
            break;
        }
    }
}

/**
 * Entry point for program
 */
int main(int argc, char* argv[])
{
    // Processes arguments (logfile id and thread count)
    int logfile_id = 0, thread_count = 0;
    char logname[sizeof(LOGNAME_TEMPLATE)];

    // Missing necessary arguments
    if (argc < 2)
    {
        fprintf(stderr, "ERR: Invalid Arguments.  Must include thread count, see README for more details.");
        return 1;
    } 
    // If args included logfile id, set here
    else if (argc >= 3)
    {
        logfile_id = atoi(argv[2]);
    }

    // Set logfile name
    if (logfile_id > 0)
    {
        sprintf(logname, LOGNAME_TEMPLATE, logfile_id);
    }
    else
    {
        sprintf(logname, LOGNAME_DEFAULT);
    }

    // Get thread count from args
    thread_count = atoi(argv[1]);

    // Initialize task manager
    tman_t* taskman = tman_init(thread_count * 2, thread_count, logname);
    // Enter process loop
    loop(taskman);
}