#include "prv_tasks.h"

tqueue_t qcreate()
{
    return (tqueue_t){0, NULL, NULL};
}

uint32_t qsize(tqueue_t* queue)
{
    return queue->size;
}

int qpop(tqueue_t* queue)
{
    // Safely handle empty queue case
    if (queue->size == 0)
    {
        return 0;
    }

    // Get head of queue
    tqueue_node_t* popped = queue->head;
    int n = popped->n;

    // Adjust queue and free popped
    queue->head = popped->next;
    queue->size--;
    free(popped);

    return n;
}

void qpush(tqueue_t* queue, int n)
{
    // Init new node
    tqueue_node_t* item = malloc(sizeof(tqueue_node_t));
    item->n = n;

    // Place node properly in queue
    if (queue->size == 0)
    {
        queue->head = item;
    }
    else
    {
        queue->tail->next = item;
    }
    // Update size and tail
    queue->size++;
    queue->tail = item;
}