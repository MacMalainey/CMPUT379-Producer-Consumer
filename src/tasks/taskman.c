#include <pthread.h>

#include "prv_tasks.h"
#include "../tasks.h"

/**
 * Returns the current as represented by gettimeofday
 * 
 * Returns:
 *  - struct timeval current_time: current time
 */
struct timeval get_actual_time()
{
    struct timeval ctime;
    gettimeofday(&ctime, NULL);

    return ctime;
}

/**
 * Returns the difference between two times as a float representing difference in seconds
 * 
 * Returns:
 *  - float difference: difference in time as seconds
 */
float timeval_diff_to_float(struct timeval start, struct timeval end)
{
    float result = 0;
    result += end.tv_sec - start.tv_sec;
    result += (end.tv_usec - start.tv_usec) / MICROSECONDS_TO_SECONDS_FACTOR;
    return result;
}

/**
 * Log an event to the statistics reports
 * 
 * Params:
 *  - tconf_t* conf: handle to shared config
 *  - uint32_t id: thread id
 *  - char* event: event to log
 */
void log_event_to_report(tconf_t* conf, uint32_t id, char* event)
{
    // Update stats on a WORK event
    if (strcmp(event, WORK_EVENT) == 0)
    {
        conf->master_report.works++;
    }
    // Update stats on a ASK event
    else if (strcmp(event, ASK_EVENT) == 0)
    {
        conf->master_report.asks++;
    }
    // Update stats on a RECIEVE event
    else if (strcmp(event, RECIEVE_EVENT) == 0)
    {
        conf->master_report.recieves++;
    }
    // Update stats on a COMPLETE event
    else if (strcmp(event, COMPLETE_EVENT) == 0)
    {
        conf->master_report.completes++;
        conf->thread_reports[id - 1]++;
    }
    // Update stats on a SLEEP event
    else if (strcmp(event, SLEEP_EVENT) == 0)
    {
        conf->master_report.sleeps++;
    }
}

/**
 * Log an event to the log file
 * 
 * Params:
 *  - tconf_t* conf: handle to shared config
 *  - uint32_t id: thread id
 *  - int q: number of elements in queue (SET TO NO_DATA TO NOT SHOW)
 *  - char* event: event to log
 *  - int n: value of data (SET TO NO_DATA TO NOT SHOW)
 */
void log_event(tconf_t* conf, uint32_t id, int q, char* event, int n)
{
    // Get current execution time (actual time, not cpu cycles)
    float time = timeval_diff_to_float(conf->start_time, get_actual_time());
    // Handle q and n data if any
    char qstr[5] = "    ";
    char nstr[5] = "    ";
    if (q != NO_DATA) { sprintf(qstr, "Q=%2d", q); }
    if (n != NO_DATA) { sprintf(nstr, "%4d", n); }

    /* CRITICAL SECTION */
    pthread_mutex_lock(&(conf->logger_lock));

    // Update report stats
    log_event_to_report(conf, id, event);

    // Print to log file
    fprintf(conf->logfile, "%.3f ID=%2d %4s %-8s %4s\n", time, id, qstr, event, nstr);

    pthread_mutex_unlock(&(conf->logger_lock));
    /* END CRITICAL SECTION */
}

/**
 * Worker thread request access to new data
 * 
 * Params:
 *  - tsub_t* sub: worker thread information handle
 * 
 * Returns:
 *  - int data: data to process or NO_DATA if manager is closed
 */
int consumer_request(tsub_t* sub)
{
    tconf_t* conf = sub->config;

    log_event(conf, sub->id, NO_DATA, ASK_EVENT, NO_DATA);

    /* CRITICAL SECTION */
    pthread_mutex_lock(&(conf->buffer_lock));

    // Block until queue is not empty or manager closes
    while (qsize(&(conf->buffer)) == 0)
    {
        if (*(sub->complete))
        {
            pthread_mutex_unlock(&(conf->buffer_lock));
            return NO_DATA;
        }
        pthread_cond_wait(&(conf->no_tasks_cond), &(conf->buffer_lock));
    }

    // Get data from queue
    int n = qpop(&(conf->buffer));

    log_event(conf, sub->id, qsize(&(conf->buffer)), RECIEVE_EVENT, n);

    // Signal that data was removed from the queue
    pthread_cond_signal(&(conf->full_buffer_cond));
    // Note: while it is only useful for the above to be run if the queue was previously full
    // the cost of running all on every buffer grab is negligible for the scope of the program

    pthread_mutex_unlock(&(conf->buffer_lock));
    /* END CRITICAL SECTION */

    return n;
}

/**
 * Worker thread control loop
 * 
 * Params:
 *  - void* sub: of tsub_t* type, worker thread information
 * 
 * Returns:
 *  - void* empty: NULL
 */
void* consumer_loop(void* subscription)
{
    // Free up memory on heap immediately because
    // it's easier to keep this data on the stack
    tsub_t sub;
    memcpy(&sub, subscription, sizeof(tsub_t));
    free(subscription);

    // Loop until no data is returned
    int n = consumer_request(&sub);
    while (n != NO_DATA)
    {
        Trans(n); // Do work
        log_event(sub.config, sub.id, NO_DATA, COMPLETE_EVENT, n); // Log work complete
        n = consumer_request(&sub); // Get new work
    }

    return NULL;
}

/**
 * Print summary stats report to the log file
 * 
 * Params:
 *  - tman_t* manager: manager to print summary data from
 *  - float total_execution_time: total execution time of program
 */
void log_summary_report(tman_t* manager, float total_execution_time)
{
    // Get config and reports
    tconf_t* conf = &(manager->config);
    tmreport_t* master_report = &(conf->master_report);

    /* Normally to gain access to this logfile we would need to use the
       synchronization methods as the logfile is a shared resource.

       But since this function SHOULD ONLY BE CALLED WHEN ALL WORKER THREADS ARE CLOSED
       this is not a concern, if we weren't able to confirm that requirement
       the lines below should be a critical section and properly synchronized. */

    // Log summary stats
    fprintf(conf->logfile, "Summary:\n");
    fprintf(conf->logfile, "    %-10s %4d\n", WORK_EVENT, master_report->works);
    fprintf(conf->logfile, "    %-10s %4d\n", ASK_EVENT, master_report->asks);
    fprintf(conf->logfile, "    %-10s %4d\n", RECIEVE_EVENT, master_report->recieves);
    fprintf(conf->logfile, "    %-10s %4d\n", COMPLETE_EVENT, master_report->completes);
    fprintf(conf->logfile, "    %-10s %4d\n", SLEEP_EVENT, master_report->sleeps);

    // Log thread stats
    for (uint32_t i = 0; i < manager->thread_count; i++)
    {
        fprintf(conf->logfile, "    Thread %2d  %4d\n", i + 1, conf->thread_reports[i]);
    }

    // Calculate and log transactions per second
    fprintf(conf->logfile, "Transactions per second: %.2f", (1.0f*master_report->works)/total_execution_time);
}

tman_t* tman_init(uint32_t buffer_size, uint32_t thread_count, char* logname)
{
    // Allocate variables
    tman_t* manager = malloc(sizeof(tman_t));
    tconf_t* config = &(manager->config);

    // Init data buffer
    config->max_buffer_size = buffer_size;
    config->buffer = qcreate();

    // Init synchronization locks before spawning threads
    config->buffer_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    config->no_tasks_cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    config->full_buffer_cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    config->logger_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    // Open logfile
    config->logfile = fopen(logname, "w");

    // Init reports
    memset(&(config->master_report), 0, sizeof(tmreport_t));
    config->thread_reports = malloc(sizeof(uint32_t) * thread_count);
    gettimeofday(&(config->start_time), NULL);

    // Spawn threads
    manager->threads = malloc(sizeof(pthread_t) * thread_count);
    for (int i = 0; i < thread_count; i++)
    {
        // Setup worker handle
        tsub_t* sub = malloc(sizeof(tsub_t));
        sub->id = i + 1;
        sub->config = config;
        sub->complete = &(manager->complete);

        // Spawn the new worker, will immediately start working
        pthread_create(&(manager->threads[i]), NULL, consumer_loop, sub);
    }
    manager->thread_count = thread_count;

    return manager;
}

void tman_publish(tman_t* manager, int n)
{
    tconf_t* config = &(manager->config);

    /* CRITICAL SECTION */
    pthread_mutex_lock(&(config->buffer_lock));

    log_event(config, 0, qsize(&(config->buffer)), WORK_EVENT, n);

    // Block if queue is full
    while (qsize(&(config->buffer)) >= config->max_buffer_size)
    {
        pthread_cond_wait(&(config->full_buffer_cond), &(config->buffer_lock));
    }

    qpush(&(config->buffer), n); // Push to queue
    pthread_cond_signal(&(config->no_tasks_cond)); // Unblock any threads on queue empty

    pthread_mutex_unlock(&(config->buffer_lock));
    /* END CRITICAL SECTION */
}

void tman_sleep(tman_t* manager, int length)
{
    log_event(&(manager->config), 0, NO_DATA, SLEEP_EVENT, length);
    Sleep(length);
}

void tman_close(tman_t* manager)
{
    tconf_t* config = &(manager->config);

    log_event(config, 0, NO_DATA, END_EVENT, NO_DATA);

    /* CRITICAL SECTION */
    pthread_mutex_lock(&(config->buffer_lock));

    // Update control flag
    manager->complete = true;

    // Unblock all threads that are currently waiting on a new task
    pthread_cond_broadcast(&(config->no_tasks_cond));

    pthread_mutex_unlock(&(config->buffer_lock));
    /* END CRITICAL SECTION */

    // Wait for threads to close
    for (uint32_t i = 0; i < manager->thread_count; i++)
    {
        pthread_join(manager->threads[i], NULL);
    }

    // Print the final summary
    log_summary_report(manager, timeval_diff_to_float(config->start_time, get_actual_time()));

    // Clean up resources
    free(config->thread_reports);
    fclose(config->logfile);
}
