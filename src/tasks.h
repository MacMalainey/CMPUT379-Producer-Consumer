/**
 * Task manager handle
 * Handles issuing tasks to worker threads
 */
struct task_manager;
typedef struct task_manager tman_t;

/**
 * Create and initialize a new task manager instance
 * 
 * Params:
 *  - uint32_t buffer_size: max size of the data buffer
 *  - uint32_t thread_count: number of worker threads to spawn
 *  - char* logname: filename of the logfile to log to
 * 
 * Returns:
 *  - tman_t* manager: new task manager configured by paramaters
 */
tman_t* tman_init(uint32_t buffer_size, uint32_t thread_count, char* logname);

/**
 * Publish transaction to a task manager for worker threads to handle
 * 
 * Params:
 *  - tman_t* manager: task manager to publish transaction to
 *  - int data: transaction data
 */
void tman_publish(tman_t* manager, int data);

/**
 * Sleep thread, also logs sleep to logfile the given manager uses
 * 
 * Params:
 *  - tman_t* manager: task manager to use to sleep
 *  - int length: hundredths of a second to sleep
 */
void tman_sleep(tman_t* manager, int length);

/**
 * Close a given task manager
 * 
 * Waits for worker threads to finish queue and close
 * Prints a summary report in the logfile when done
 * 
 * Params:
 *  - tman_t* manager: task manager to close
 */
void tman_close(tman_t* manager);
