/**
* Created By Fabert Charles
*
* Houses function definitions of multiTHREAD_SAFE safe queue
*/

/*Related Libraries*/
#include "queue/queue.h"

#ifdef THREAD_SAFE
    #include <pthread.h>
#endif

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

    #ifdef THREAD_SAFE
        pthread_mutex_t key;
        pthread_cond_t push_condition;
        pthread_cond_t pop_condition;
    #endif
} Queue;


#ifdef THREAD_SAFE
void WakeThreads( Queue* q ) {
    if ( q == NULL ) {
        return;
    }

    pthread_cond_broadcast( &( q->push_condition ) );
    pthread_cond_broadcast( &( q->pop_condition ) );
}
#endif

bool Push( Queue* q, void *elem ) {
    Node node = NewNode( elem );

    if ( node == NULL ) {
        return false;
    }

    if ( q->current_length == 0 ) {
        q->head = node;

        #ifdef THREAD_SAFE
            pthread_cond_signal( &( q->pop_condition ) );
            pthread_cond_signal( &( q->push_condition ) );
        #endif
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

    #ifdef THREAD_SAFE
        if ( q->current_length == q->size )
        {
            pthread_cond_signal( &( q->push_condition ) );
            pthread_cond_signal( &( q->pop_condition ) );
        }
    #endif

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

    #ifdef THREAD_SAFE
        pthread_mutex_init( &( queue->key ), NULL );
        pthread_cond_init( &( queue->push_condition ), NULL );
        pthread_cond_init( &( queue->pop_condition ), NULL );
    #endif

    return queue;
}

void QueueDelete( Queue **q ) {
    if ( q == NULL || *q == NULL ) {
        return;
    }

    #ifdef THREAD_SAFE
        // Destroy the mutexes
        pthread_mutex_destroy( &( ( *q )->key ) );
        pthread_cond_destroy( &( ( *q )->pop_condition ) );
        pthread_cond_destroy( &( ( *q )->push_condition ) );
    #endif

    free( *q );

    *q = NULL;
}

bool QueuePush( Queue *q, void *elem ) {
    if ( ( q == NULL ) || ( elem == NULL ) ) {
        return false;
    }

    bool result = false;

    #ifdef THREAD_SAFE
        pthread_mutex_lock( &( q->key ) );

        // wait for buffer not full
        while ( ( q->current_length == q->size ) && !q->queue_shutting_down ) {
            pthread_cond_wait( &( q->push_condition ), &( q->key ) );
        }

        //signal to quit is received
        if ( q->queue_shutting_down ) {
            pthread_mutex_unlock( &( q->key ) );
            return false;
        }

        result = Push( q, elem );

        pthread_mutex_unlock( &( q->key ) );

    #else
        if ( q->current_length == q->size ) {
            return false;
        }

        result = Push( q, elem );
    #endif

    return result;
}

bool QueuePop( Queue *q, void **elem ) {
    if ( ( q == NULL ) || ( elem == NULL ) ) {
        return false;
    }

    #ifdef THREAD_SAFE
        pthread_mutex_lock( &( q->key ) );

        while ( ( q->current_length == 0 ) && !q->queue_shutting_down ) {
            pthread_cond_wait( &( q->pop_condition ), &( q->key ) );
        }

        // If an end signal is received
        if ( q->queue_shutting_down ) {
            *elem = NULL;
            pthread_mutex_unlock ( &( q->key ) );
            return false;
        }

        Pop( q, elem );

        pthread_mutex_unlock( &( q->key ) );
    
    #else
        if ( q->current_length == 0 ) {
            return false;
        }

        Pop( q, elem );
    #endif

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

    #ifdef THREAD_SAFE
        WakeThreads( q );
    #endif
}

size_t QueueLength( Queue *q ) {
    size_t size = 0;

    #ifdef THREAD_SAFE
        pthread_mutex_lock( &( q->key ) ); // LOCK MUTEX
        size = q->current_length;
        pthread_mutex_unlock( &( q->key ) );
    #else
        size = q->current_length;
    #endif

    return size;
}

size_t QueueMaxSize( Queue *q ) {
    return q->size;
}