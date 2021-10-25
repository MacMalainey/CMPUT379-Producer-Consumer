#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

/**
 * Flag for no data
 */
#define NO_DATA -1

/**
 * Logname for a transaction ask event
 */
#define ASK_EVENT "Ask"
/**
 * Logname for a transaction recieve event
 */
#define RECIEVE_EVENT "Recieve"
/**
 * Logname for a transaction being added to queue event
 */
#define WORK_EVENT "Work"
/**
 * Logname for a sleep event
 */
#define SLEEP_EVENT "Sleep"
/**
 * Logname for a transaction complete event
 */
#define COMPLETE_EVENT "Complete"
/**
 * Logname for manager close event
 */
#define END_EVENT "End"

/**
 * Scale factor for representing microseconds as seconds
 */
#define MICROSECONDS_TO_SECONDS_FACTOR 1000000.0f

/**
 * Data node in task queue
 * 
 * Node for linked-list implementation of the task queue
 */
typedef struct task_queue_node {
    /**
     * Points to the next node in the queue
     * If no data has been added to the queue, points to NULL
     */
    struct task_queue_node* next;

    /**
     * Data at node
     */
    int n;
} tqueue_node_t;

/**
 * Task queue
 * Stores transaction tasks as a queue, supports pushing and popping
 * 
 * Implemented as a linked list
 */
typedef struct task_queue {
    /**
     * Number of nodes in the queue
     */
    uint32_t size;

    /**
     * Current head of the queue
     */
    tqueue_node_t* head;
    /**
     * Curent tail of the queue
     */
    tqueue_node_t* tail;
} tqueue_t;

/**
 * Statistics tracker for logging to task manager's log
 * 
 * Stores number of events fired
 */
typedef struct task_master_report {
    /**
     * Number of work events
     */
    uint32_t works;
    /**
     * Number of ask events
     */
    uint32_t asks;
    /**
     * Number of recieve events
     */
    uint32_t recieves;
    /**
     * Number of complete events
     */
    uint32_t completes;
    /**
     * Number of sleep events
     */
    uint32_t sleeps;
} tmreport_t;

/**
 * Common config object for accessing information required across all worker threads
 * and the task manager including logging, data buffer, and synchronization locks
 */
typedef struct task_config {
    /* Buffer Synchronization */

    /**
     * Mutex for accessing data buffer
     */
    pthread_mutex_t buffer_lock;
    /**
     * Condition for blocking when the queue is currently empty, signaled when items are added to the queue
     */
    pthread_cond_t no_tasks_cond;
    /**
     * Condition for blocking when the queue is currently full, signaled when items are remnoved from the queue
     */
    pthread_cond_t full_buffer_cond;

    /* Logger */
    
    /**
     * Mutex for accessing logger
     */
    pthread_mutex_t logger_lock;
    /**
     * File resource to log events to
     */
    FILE* logfile;

    /* Data Queue */

    /**
     * Maximum size the buffer can be
     */
    uint32_t max_buffer_size;
    /**
     * The data buffer
     */
    tqueue_t buffer;

    /* Reports */

    /**
     * Master statistics information
     */
    tmreport_t master_report;
    /**
     * Report of work completed per thread
     */
    uint16_t* thread_reports;
    /**
     * Timestamp of when the task manager was initialized
     */
    struct timeval start_time;
} tconf_t;

/**
 * Handle for worker threads to get access to the shared configuration and local information
 */
typedef struct task_subscription {
    /**
     * Thread ID
     */
    uint32_t id;

    /**
     * Shared config
     */
    tconf_t* config;
    /**
     * Status flag to check if the manager has requested to be closed
     */
    bool* complete;
} tsub_t;

struct task_manager {
    /* Threads */
    
    /**
     * Total number of worker threads spawned
     */
    uint32_t thread_count;
    /**
     * Handles for every thread
     */
    pthread_t* threads;

    /* Config */

    /**
     * Shared config
     */
    tconf_t config;
    /**
     * Flag for signalling to worker threads when the manager has been closed
     */
    bool complete;
};

/* QUEUE PROCEDURES */

/**
 * Creates an empty task queue
 * 
 * Returns:
 *  - tqueue_t queue: newly created empty task queue
 */
tqueue_t qcreate();
/**
 * Checks the size of a given queue
 * 
 * Params:
 *  - tqueue_t* queue: queue to get size of
 * 
 * Returns:
 *  - uint32_t size: number of elements in queue
 */
uint32_t qsize(tqueue_t* queue);
/**
 * Pops the last item from the queue
 * 
 * Params:
 *  - tqueue_t* queue: queue to get element from
 * 
 * Returns:
 *  - int data: item popped from queue, return NO_DATA if queue is empty
 */
int qpop(tqueue_t* queue);
/**
 * Pushes an item to the queue
 * 
 * Params:
 *  - tqueue_t* queue: queue to push item to
 *  - int n: item to push to the queue
 */
void qpush(tqueue_t* queue, int n);

/* ASSIGNMENT PROVIDED INSTRUCTIONS */

/**
 * Function to handle transactions
 */
void Trans(int n);
/**
 * Function to perform sleep
 */
void Sleep(int n);