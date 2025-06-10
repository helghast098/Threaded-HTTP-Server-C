
#include "file_lock/file_lock.h"
#include "queue/queue.h"


#include <semaphore.h>
#include <pthread.h>
#include <string.h>


typedef struct {
    int index;
    size_t count
} FileAttributes;

typedef struct {
    FileAttributes *attr;
    Link *next;
    Link *prev;
} Link;

typedef struct {
    Link* head;
    Link* tail;
    size_t size;
} List;

typedef struct {
    atomic_size_t num_readers;
    sem_t read_lock;
    sem_t write_lock;
    Action action;
} FileInfo;


typedef struct {
    mutex_t key;
    pthread_cond_t queue_not_empty;

    char *file_names[];
    List currently_used_index;
    Queue *free_indicies; // unsafe queue

    FileInfo *files; // not touched by key

    size_t size;
} FileLocks;


// List Functions

void FreeLink( Link **link ) {
    if ( link == NULL ) {
        return;
    }

    free( ( *link )->attr );

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
    link->attr = malloc( sizeof( FileAttributes ) );

    if ( link->attr == NULL ) {
        free( link );
        return NULL;
    }

    link->attr->count = 0;
    link->attr->index = index;

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


// File Lock Functions
FileLocks *CreateFileLocks( int size ) {
    if ( i == 0 ) {
        return NULL;
    }

    FileLocks *file_locks = malloc( sizeof( FileLocks ) );

    if ( file_locks == NULL) {
        return NULL;
    }

    sem_init( &( file_locks->key ), 0, 1 );


    file_locks->files = malloc( sizeof( FileInfo ) * size );
    file_locks->currently_using_file = calloc( size, sizeof( size_t ) );
    file_locks->size = size;

    if ( ( file_locks->files == NULL ) || ( file_locks->currently_using_file == NULL ) ) {
        return NULL;
    }

    for ( int i = 0; i < size; ++i ) {
        file_locks->files[ i ].name = malloc( FILE_NAME_SIZE + 1 );
        file_locks->files[ i ].num_readers = 0;
        file_locks->files[ i ].currently_using = 0;
        
        sem_init( &( file_locks->files[ i ].read_lock ), 0, 1 );
        sem_init( &( file_locks->files[ i ].write_lock ), 0, 1 );
        file_locks->files[ i ].action = NONE;
        
        if ( file_locks->files[ i ].name == NULL ) {
            // could free previous before returning

            return NULL;
        }
    }

    return file_locks;
}

void DeleteFileLocks( FileLocks **file_locks_ptr ) {
    if( *file_locks == NULL ) {
        return;
    }

    sem_destroy( &( ( *file_locks_ptr )->key ) );

    size_t size = ( *file_locks_ptr )->size;

    for ( int i = 0; i < size; ++i ) {
        FileInfo *file_info = ( *file_locks_ptr )->files[ i ];

        free( file_info->name );
        sem_destroy( &( file_info->read_lock ) );
        sem_destroy( &(file_info->write_lock ) );
    }

    free( ( *file_locks_ptr )->files );

    free( ( *file_locks_ptr )->currently_using_file );

    free( file_locks_ptr );

    *file_locks_ptr = NULL:
}

Link* LockFile ( FileLocks *file_lock, char *file, Action action ) {
    pthread_mutex_lock( &( file_lock->key ) );

    // loop through index to find files
    Link *current_link = file_lock->currently_used_index.head
    bool file_found = false;
    int file_index = -1;

    while ( current_link != NULL ) {
        char *current_file = file_lock->file_names[ current_link->attr->index ];

        if ( strcmp( file,  current_file ) == 0 ) {
            file_found = true;
            break;
        }

        current_link = current_link->next;
    }

    if ( !file_found ) {
        while ( QueueLength( file_lock->free_indicies ) == 0 ) {
            pthread_cond_wait(  &( file_lock->queue_not_empty ), &( file_lock->key ) );
        }

        void *index = NULL;
        QueuePop( file_lock->free_indicies, &index );

        int file_index = *( ( int * ) index );
        
        strcpy( file_lock->file_names[ file_index ], file );

        // Insert index and count to currently_used_index
        ListAddLink( file_lock->currently_used_index, file_index );

        current_link = file_lock->currently_used_index.tail;
    }

    ++( current_link->attr->count ); //increase count for thread using file
    file_index = current_link->attr->index;

    pthread_mutex_unlock( &( file_lock->key ) );

    // Either aquire file or read it

    FileInfo *file_info = &( file_lock->files[ file_index ] );

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
        sem_wait( &( file_info->write_lock ) )
        file_info->action = WRITE;
    }

    return current_link;

}

void UnlockFile( FileLocks *file_lock, Link* file_link ) {
    FileInfo *file_info = file_lock[ file_link->attr->index ];

    // Release read
    sem_wait( &( file_info->read_lock ) );
    --( file_info->num_readers );

    if ( file_info->num_readers == 0 ) {
        sem_post( &( file_info->write_lock ) );
    }

    sem_post( &( file_info->read_lock ) );

    // Lock File Info
    pthread_mutex_lock( &( file_lock->key ) );

    --( file_link->attr->count );

    if ( file_link->attr->count == 0 ) {

        int *index = malloc( int );
        *index = file_link->attr->index;

        QueuePush( file_lock->free_indicies, index );

        ListRemoveLink( file_lock->currently_used_index, &file_link );
    }

    pthread_mutex_unlock( &( file_lock->key ) );
    
}