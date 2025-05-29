/**
* Created By Fabert Charles
*
* Houses function declarations for multithreaded safe queue
*/

#ifndef SIMPLEHTTP_QUEUE_H_
#define SIMPLEHTTP_QUEUE_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>



/*Type Definitions*/
typedef struct Queue Queue; //Defines the struct queue as Queue

/*Function Declarations */
/** @brief Allocates memory for queue an returns a pointer to it
*   @param size: How big the queue is
*   @return Queue
**/
Queue* QueueNew(int size); // Creates new queue of size

/**
 * @note 1st: Call QueueShutdown() 2: Wait until all threads have joined and then empty queue using QueuePop() 3: Finally call QueueDelete()
 * @brief Deletes queue and releases the memory
*   @param q: Address of the pointer to the queue
*   @return void
*/
void QueueDelete(Queue** q);

/** @brief Pushes elements to the queue of type void *
*   @param q:  Pointer to queue
*   @param elem:  element to push to queue
*   @return true on success, false on failure
*/
bool QueuePush(Queue* q, void* elem);

/** @brief Pop elements off the queue of type void *
*   @param q:  Pointer to queue
*   @param elem:  Holds the pointer to the pointer of the popped element
*   @return true on success, false on failure
*/
bool QueuePop(Queue* q, void** elem);

/**
 *  
 * @brief Just alerts one thread waiting on push condition
*   @param q:  Pointer to a queue
*   @return void
*/

/** @brief Just Alerts threads to wake up and exit pop and push functions
*   @param q:  Pointer to a queue
*   @return void
*/
void QueueShutDown( Queue *q);

/** @brief Gets current length of queue
*   @param q: Pointer to a queue
*   @return length (int)
*/

size_t QueueLength(Queue* q );

size_t QueueMaxSize( Queue* q );


#endif
