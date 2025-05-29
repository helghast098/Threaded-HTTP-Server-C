/**
* Created By Fabert Charles
*
* Houses function definitions of multithreaded safe queue
*/

/*Related Libraries*/
#include "queue/queue.h"

#include <pthread.h>

typedef struct NodeObj {
    void *data;
    struct NodeObj *previous_node;
} NodeObj;

typedef NodeObj *Node;

Node NewNode( void *elem ) {
    Node node = malloc( sizeof( NodeObj ) );

    if ( node == NULL ) {
        return NULL;
    }

    node->data = elem;
    node->previous_node = NULL;

    return node;
}


typedef struct Queue {
    atomic_bool queue_shutting_down;
    size_t size;
    size_t current_length;
    Node head;
    Node tail;
    pthread_mutex_t key;
    pthread_cond_t push_condition;
    pthread_cond_t pop_condition;
} Queue;


void WakeThreads( Queue* q ) {
    if ( q == NULL ) {
        return;
    }

    pthread_cond_broadcast( &( q->push_condition ) );
    pthread_cond_broadcast( &( q->pop_condition ) );
}

bool Push( Queue* q, void *elem ) {
    Node node = NewNode( elem );
    if ( node == NULL ) {
        return false;
    }

    if ( q->current_length == 0 ) {
        q->head = node;
        pthread_cond_signal( &( q->pop_condition ) );
        pthread_cond_signal( &( q->push_condition ) );
    } 
    else {
        q->tail->previous_node = node; // Set tail previous node to new node
    }

    q->tail = node;

    q->current_length += 1;

    return true;
}

void Pop( Queue *q, void **elem ) {
    *elem = q->head->data; // saving the data for user
    q->head->data = NULL; // setting node data to NULL

    Node head_holder = q->head;
    q->head = q->head->previous_node;

    if ( q->current_length == 1 )
    {
        q->tail = NULL;
    }

    if ( q->current_length == q->size )
    {
        pthread_cond_signal( &( q->push_condition ) );
        pthread_cond_signal( &( q->pop_condition ) );
    }

    free( head_holder );

    q->current_length -= 1;
}



Queue *QueueNew( int size ) {
    Queue *queue = malloc( sizeof( Queue ) );

    if ( queue == NULL ) {
        return NULL;
    }

    queue->queue_shutting_down = false;
    queue->size = size;
    queue->current_length = 0;
    queue->head = NULL;
    queue->tail = NULL;

    pthread_mutex_init( &( queue->key ), NULL );
    pthread_cond_init( &( queue->push_condition ), NULL );
    pthread_cond_init( &( queue->pop_condition ), NULL );

    return queue;
}

void QueueDelete( Queue **q ) {
    if ( q == NULL || *q == NULL ) {
        return;
    }

    // Destroy the mutexes
    pthread_mutex_destroy( &( ( *q )->key ) );
    pthread_cond_destroy( &( ( *q )->pop_condition ) );
    pthread_cond_destroy( &( ( *q )->push_condition ) );

    free( *q );

    *q = NULL;
}

bool QueuePush( Queue *q, void *elem ) {
    if (q == NULL) {
        return false;
    }

    // locking mutex
    pthread_mutex_lock(&(q->key));

    // wait for buffer not full
    while ( ( q->current_length == q->size ) && !q->queue_shutting_down ) {
        pthread_cond_wait( &( q->push_condition ), &( q->key ) );
    }

    //signal to quit is received
    if ( q->queue_shutting_down ) {
        pthread_mutex_unlock( &( q->key ) );
        return false;
    }

    //== START CRITICAL SECTION ==

    if ( !Push( q, elem ) ) {
        pthread_mutex_unlock( &( q->key ) );
        return false;
    }

    // == END CRITICAL SECTION ==

    pthread_mutex_unlock( &( q->key ) );
    return true;
}

bool QueuePop( Queue *q, void **elem ) {
    if ( ( q == NULL ) || ( elem == NULL ) ) {
        return false;
    }

    pthread_mutex_lock(&(q->key));

    while ( (q->current_length == 0 ) && !q->queue_shutting_down ) {
        pthread_cond_wait( &( q->pop_condition ), &( q->key ) );
    }

    // If an end signal is received
    if ( q->queue_shutting_down ) {
        *elem = NULL;
        pthread_mutex_unlock ( &( q->key ) );
        return false;
    }

    //== START CRITICAL SECTION ==

    Pop( q, elem );

    //== START CRITICAL SECTION ==
    
    pthread_mutex_unlock( &( q->key ) );

    return true;
}

void QueueShutDown( Queue *q ) {
    if ( q == NULL ) {
        return;
    }

    if ( q->queue_shutting_down ){
        return;
    }

    q->queue_shutting_down = true;

    WakeThreads( q );
}

size_t QueueLength( Queue *q ) {
    size_t size = 0;
    pthread_mutex_lock( &( q->key ) ); // LOCK MUTEX
    size = q->current_length;
    pthread_mutex_unlock( &( q->key ) );
    return size;
}

size_t QueueMaxSize( Queue *q ) {
    return q->size;
}