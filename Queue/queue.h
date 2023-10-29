/**
* Created By Fabert Charles
*
* Houses function declarations for multithreaded safe queue
*/

#ifndef QUEUE_H
#define QUEUE_H
/*Included Libraries*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

/*Extern Vars*/
extern volatile atomic_bool ev_signQuit;

/*Type Definitions*/
typedef struct queue queue_t; //Defines the struct queue as queue_t

/** @brief Allocates memory for queue an returns a pointer to it
*   @param size: How big the queue is
*   @return queue_t
**/
queue_t* queue_new(int size); // Creates new queue of size

/** @brief Deletes queue and releases the memory
*   @param q: Address of the pointer to the queue
*   @return void
*/
void queue_delete(queue_t** q);

/** @brief Pushes elements to the queue of type void *
*   @param q:  Pointer to queue
*   @param elem:  element to push to queue
*   @return true on success, false on failure
*/
bool queue_push(queue_t* q, void* elem);

/** @brief Pop elements off the queue of type void *
*   @param q:  Pointer to queue
*   @param elem:  Holds the pointer to the pointer of the popped element
*   @return true on success, false on failure
*/
bool queue_pop(queue_t* q, void** elem);

/** @brief Just alerts one thread waiting on push condition
*   @param q:  Pointer to a queue
*   @return void
*/
void queue_condition_push(queue_t* q);

/** @brief Just alerts one thread waiting on pop condition
*   @param q:  Pointer to a queue
*   @return void
*/
void queue_condition_pop(queue_t* q);

/** @brief returns current size of queue
*   @param q:  Pointer to a queue
*   @return size of queue as int
*/
int queue_size(queue_t* q);
#endif