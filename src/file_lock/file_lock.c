
#include "file_lock/file_lock.h"
#include "queue/queue.h"


#include <semaphore.h>
#include <pthread.h>
#include <string.h>


typedef struct {
    int index;
    size_t threads_currently_using;
} FileAttributes;


typedef struct Link{
    FileAttributes attr;
    struct Link *next;
    struct Link *prev;
} Link;

typedef struct {
    Link* head;
    Link* tail;
    size_t size;
} List;

typedef struct {
    size_t num_readers;
    sem_t read_lock;
    sem_t write_lock;
    Action action;
} FileInfo;

typedef struct FileLocks{
    pthread_mutex_t key;

    // Reliant on key
    pthread_cond_t queue_not_empty_cond;
    char **file_names;
    List* used_index_list;
    Queue *free_indicies_queue;

    // Not Reliant on key
    FileInfo *file_info;
    size_t size;
} FileLocks;


// List Functions PRIVATE

void FreeLink( Link **link ) {
    if ( link == NULL ) {
        return;
    }

    free( *link );

    *link = NULL;
}

Link *CreateLink( int index ) {
    Link *link = malloc( sizeof( Link ) );

    if ( link == NULL ) {
        return NULL;
    }

    link->next = NULL;
    link->prev = NULL;

    link->attr.threads_currently_using = 0;
    link->attr.index = index;

    return link;

}

void ListRemoveLink( List *list, Link **link ) {
    if ( ( list == NULL ) || ( link == NULL ) ) {
        return;
    }

    Link *prev_link = ( *link )->prev;
    Link *next_link = ( *link )->next;

    if ( prev_link != NULL ) {
        prev_link->next = next_link;
    }

    if ( next_link != NULL ) {
        next_link->prev = prev_link;
    }    

    if ( ( *link ) == list->head ) {
        list->head = next_link;
    }

    if ( ( *link ) == list->tail ) {
        list->tail = prev_link;
    }

    FreeLink( link );

    --( list->size );
} 

void ListAddLink( List *list, int index ) {
    if ( ( list == NULL ) || ( index < 0 ) ) {
        return;
    }

    Link *link = CreateLink( index );

    if ( list->head == NULL ) {
        list->tail = link;
        list->head = link;
    }
    else {
        list->tail->next = link;
        link->prev = list->tail;
        list->tail = link;
    }

    ++( list->size );
}

List *ListNew() {
    List * list = malloc( sizeof( List ) );

    if ( list == NULL ) {
        return NULL;
    }

    list->size = 0;
    list->head = NULL;
    list->tail = NULL;

    return list;
}

void ListDelete( List **list_ptr ) {
    if ( list_ptr == NULL ) {
        return;
    }

    Link *current_link = ( *list_ptr )->head;
    while ( current_link != NULL ) {
        Link *next_link = current_link->next;

        free( current_link );

        current_link = next_link;
    }

    free( *list_ptr );
    *list_ptr = NULL;
}

// File Lock Functions
FileLocks *CreateFileLocks( size_t size ) {
    if ( size == 0 ) {
        return NULL;
    }

    FileLocks *file_locks = malloc( sizeof( FileLocks ) );

    if ( file_locks == NULL) {
        return NULL;
    }

    file_locks->size = size;

    // Initializing key and Condition
    pthread_mutex_init( &( file_locks->key ), NULL );
    pthread_cond_init( &( file_locks->queue_not_empty_cond ), NULL );

    // Initializing array of file names
    file_locks->file_names = malloc( sizeof( char * ) * size );

    for ( int i = 0; i < size; ++i ) {
        char *name = malloc( sizeof( char ) *  FILE_NAME_SIZE );
        if ( name == NULL ) {
            // add cleanup code
            return NULL;
        }
        file_locks->file_names[ i ] = name;
    }

    // Initializing List
    file_locks->used_index_list = ListNew();
    
    // Initializing Queue
    file_locks->free_indicies_queue = QueueNew( size );

    for ( int i = 0; i < size; ++i ) {

        int *index = malloc( sizeof( int ) );

        *index = i;
        QueuePush( file_locks->free_indicies_queue, index );
    }

    // Initialzing array of file info
    file_locks->file_info = malloc( sizeof( FileInfo ) * size );

    for ( int i = 0; i < size; ++i ) {
        file_locks->file_info[ i ].num_readers = 0;
        sem_init( &( file_locks->file_info[ i ].read_lock ), 0, 1 );
        sem_init( &( file_locks->file_info[ i ].write_lock ), 0, 1 );
        file_locks->file_info[ i ].action = NONE;
    }

    return file_locks;
}

void DeleteFileLocks( FileLocks **file_locks_ptr ) {
    if( file_locks_ptr == NULL ) {
        return;
    }

    size_t size = ( *file_locks_ptr )->size;

    // Destroy key and condition_queue
    pthread_mutex_destroy( &( ( *file_locks_ptr )->key ) );
    pthread_cond_destroy( &( ( *file_locks_ptr )->queue_not_empty_cond ) );

    // Free memory of file_names
    for ( int i = 0; i < size; ++i ) {
        free( ( *file_locks_ptr )->file_names[i] );
    }

    free( ( *file_locks_ptr )->file_names );

    // Delete List
    ListDelete( & ( ( *file_locks_ptr )->used_index_list ) );

    // Delete Queue
    void *data = NULL;

    while ( QueueLength( ( *file_locks_ptr)->free_indicies_queue ) != 0 ) {
        QueuePop( ( *file_locks_ptr)->free_indicies_queue, &data );
        free( data );
    }

    QueueDelete( &( ( *file_locks_ptr )->free_indicies_queue ) );

    // Freeing File Attributes
    for ( int i = 0; i < size; ++i ) {
        FileInfo *file_info = &( ( *file_locks_ptr )->file_info[ i ] );

        sem_destroy( &( file_info->read_lock ) );
        sem_destroy( &(file_info->write_lock ) );
    }
    
    free( ( *file_locks_ptr )->file_info );

    free( *file_locks_ptr );

    *file_locks_ptr = NULL;
}

FileLink LockFile ( FileLocks *file_locks, const char *file, Action action ) {
    pthread_mutex_lock( &( file_locks->key ) );

    // loop through index to find files
    Link *current_link = file_locks->used_index_list->head;
    bool file_found = false;
    int file_index = -1;

    while ( current_link != NULL ) {
        char *current_file = file_locks->file_names[ current_link->attr.index ];

        if ( strcmp( file,  current_file ) == 0 ) {
            file_found = true;
            break;
        }

        current_link = current_link->next;
    }

    if ( !file_found ) {
        while ( QueueLength( file_locks->free_indicies_queue ) == 0 ) {
            pthread_cond_wait(  &( file_locks->queue_not_empty_cond ), &( file_locks->key ) );
        }

        void *index = NULL;
        QueuePop( file_locks->free_indicies_queue, &index );

        int file_index = *( ( int * ) index );
        
        free( index );

        strcpy( file_locks->file_names[ file_index ], file );

        // Insert index and threads_currently_using to used_index_list
        ListAddLink( file_locks->used_index_list, file_index );

        current_link = file_locks->used_index_list->tail;
    }

    ++( current_link->attr.threads_currently_using ); //increase threads_currently_using for thread using file
    
    pthread_mutex_unlock( &( file_locks->key ) );

    file_index = current_link->attr.index;
    FileInfo *file_info = &( file_locks->file_info[ file_index ] ); // Dont need key to read file_info


    if ( action == READ ) {
        sem_wait( &( file_info->read_lock ) );
        ++( file_info->num_readers );

        if ( file_info->num_readers == 1 ) {
            sem_wait( &( file_info->write_lock ) );
        }

        file_info->action = READ;
        sem_post( &( file_info->read_lock ) );
    }
    else if ( action == WRITE ) {
        sem_wait( &( file_info->write_lock ) );
        file_info->action = WRITE;
    }

    return current_link;

}

void UnlockFile( FileLocks *file_lock, FileLink *file_link_ptr ) {
    
    if ( ( file_lock == NULL ) || ( file_link_ptr == NULL ) ) {
        return;
    }

    FileInfo *file_info = & ( file_lock->file_info[ ( *file_link_ptr )->attr.index ] );

    // Release read
    if ( file_info->action == READ ) {
        sem_wait( &( file_info->read_lock ) );
        --( file_info->num_readers );

        if ( file_info->num_readers == 0 ) {
            sem_post( &( file_info->write_lock ) );
        }

        sem_post( &( file_info->read_lock ) );
    }
    else if ( file_info->action == WRITE ) {
        sem_post( &( file_info->write_lock ) );
    }

    // Lock File Info
    pthread_mutex_lock( &( file_lock->key ) );

    --( ( *file_link_ptr )->attr.threads_currently_using );

    if ( ( *file_link_ptr )->attr.threads_currently_using == 0 ) {

        int *index = malloc( sizeof( int ) );
        *index = ( *file_link_ptr )->attr.index;

        QueuePush( file_lock->free_indicies_queue, index );

        ListRemoveLink( file_lock->used_index_list,  file_link_ptr );

        pthread_cond_signal( &( file_lock->queue_not_empty_cond ) );
    }

    pthread_mutex_unlock( &( file_lock->key ) );
    
}