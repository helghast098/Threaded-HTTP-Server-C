/**
* Created By Fabert Charles
*
* Houses function definitions of multithreaded safe queue
*/

/*Libraries Included*/
#include "Queue/queue.h"

/*Type Definitions*/
typedef struct NodeObj {
    void *data;
    struct NodeObj *prevNode;
} NodeObj;

typedef NodeObj *Node;

// Defines queue
typedef struct queue {
    int size;
    int current_length;
    Node Head;
    Node Tail;
    pthread_mutex_t key; // For mutuex
    pthread_cond_t pushCond; // For push condition
    pthread_cond_t popCond; // For pop condition
} queue;

/** @brief Creates a new Node
*   @param elem:  The data to store node
*   @return Node: created node
*/
Node newNode(void *elem) {
    Node N = malloc(sizeof(NodeObj));
    // Checks if malloc worked
    if (N == NULL) {
        return NULL;
    }
    N->data = elem;
    N->prevNode = NULL;
    return N;
}

/*Function Definitions*/
queue_t *queue_new(int size) {
    queue_t *q_holder = malloc(sizeof(queue_t)); // creates the queue

    // Checks if malloc worked
    if (q_holder == NULL) {
        return q_holder;
    }
    q_holder->size = size; // Set the queue size
    q_holder->current_length = 0; // set current length
    q_holder->Head = NULL;
    q_holder->Tail = NULL;

    // Initializing p thread variables
    pthread_mutex_init(&(q_holder->key), NULL);
    pthread_cond_init(&(q_holder->pushCond), NULL);
    pthread_cond_init(&(q_holder->popCond), NULL);
    return q_holder;
}

void queue_delete(queue_t **q) {
    // Checking for valid pointer and values
    if (q != NULL && *q != NULL) {
        int num_of_elem = (*q)->current_length;
        void *trash;

        // removes elements of queue
        for (int i = 0; i < num_of_elem; ++i) {
            queue_pop(*q, &trash);
        }

        // Destroy the mutexes
        pthread_mutex_destroy(&((*q)->key));
        pthread_cond_destroy(&((*q)->popCond));
        pthread_cond_destroy(&((*q)->pushCond));

        // free memory
        free(*q);

        // setting q to null
        *q = NULL;
    }
}

bool queue_push(queue_t *q, void *elem) {
    // need to create new node
    // incrementing current_length  WATCH OUT FOR THREADS HERE

    // If valid pointer to queue
    if (q == NULL) {
        return false;
    } else {
        // add a safe mutex here
        pthread_mutex_lock(&(q->key)); // LOCK MUTEX
        int currentLen = q->current_length;
        int size = q->size;

        // wait for buffer not full
        while ((currentLen >= size) && !ev_signQuit) {
            pthread_cond_wait(&(q->pushCond), &(q->key)); // wait for condition
            currentLen = q->current_length;
        }

        // If a signal to quit is recieved
        if (ev_signQuit) {
            pthread_cond_signal(&(q->pushCond));
            pthread_mutex_unlock(&(q->key));
            return false;
        }

        // Critical Section
        Node N = newNode(elem); // creates new Node
        if (N == NULL) {
            pthread_mutex_unlock(&(q->key));
            return false;
        }

        // the list is empty
        q->current_length += 1;

        if (currentLen == 0) {
            q->Head = N;
            q->Tail = N;
            pthread_cond_signal(&(q->popCond));
        } else {
            q->Tail->prevNode = N; // Set tail prevNode to new node
            q->Tail = N; // Set new node to tail position
        }
        pthread_mutex_unlock(&(q->key)); // UNLOCK MUTEX
    }
    return true;
}

bool queue_pop(queue_t *q, void **elem) {
    // Checking for valid queue and elem
    if ((q == NULL) || (elem == NULL))
        return false;
    else {
        // need to lock this section
        pthread_mutex_lock(&(q->key)); // LOCK MUTEX
        int currentLen = q->current_length;

        while ((currentLen == 0) && !ev_signQuit) {
            pthread_cond_wait(&(q->popCond), &(q->key));
            currentLen = q->current_length;
        }

        // If a end signal is received
        if (ev_signQuit) {
            pthread_mutex_unlock(&(q->key));
            pthread_cond_signal(&(q->popCond));
            return false;
        }

        // if there's only 1 element
        *elem = q->Head->data; // save the data
        q->Head->data = NULL; // setting node data to NULL

        if (currentLen == 1) {
            q->Head->prevNode = NULL; // setting node prev data to NULL
            free(q->Head); // freeing Node
            q->Head = NULL;
            q->Tail = NULL;
        } else {
            Node Holder = q->Head;
            q->Head = Holder->prevNode;
            Holder->prevNode = NULL;
            free(Holder);
        }
        q->current_length -= 1;
        pthread_cond_signal(&(q->pushCond));
        pthread_mutex_unlock(&(q->key)); // UNLOCK MUTEX
    }
    return true;
}

void queue_condition_pop(queue_t *q) {
    pthread_cond_signal(&(q->popCond));
}

void queue_condition_push(queue_t *q) {
    pthread_cond_signal(&(q->pushCond));
}

int queue_size(queue_t *q) {
    int size = 0;
    pthread_mutex_lock(&(q->key)); // LOCK MUTEX
    size = q->current_length;
    pthread_mutex_unlock(&(q->key));
    return size;
}
