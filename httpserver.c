/**
* Created By Fabert Charles
*
* HTTP SERVER File
*/

/*Included Libraries*/
// For opening file
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// For closing file
#include <unistd.h>

// For error handling
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <signal.h>

// For Threads and atomic
#include <stdatomic.h>
#include <pthread.h>
#include <semaphore.h>

// Included Custom Libraries
#include "bind.h"
#include "Request_format/request_format.h"
#include "Http_methods/http_methods.h"
#include "Queue/queue.h"

/*Type Definitions*/
// Struct for holding a single uri
typedef struct lock_file {
    int numThreads; // Number of threads that are present here
    char URI[100];
    int numReaders; // How many threads are reading the file
    sem_t numReadKey; // Key for readers to read file
    sem_t fileWrite; // semaphore for file write
}lock_file;

// Struct to hold the list of uris
typedef struct URI_lock{
    int size;
    sem_t changeList;
    lock_file *files;
}URI_lock;

// Struct to signal threads to finissh
typedef struct signalThreads {
    int size;
    atomic_bool* finishThreads; // Holds an array for finished threads
}signalThreads;


/*Global Vars*/
URI_lock* g_uriLocks; // pointer to the uri locks

signalThreads* g_signalThreads; // pointer to signal threads

//Flag for interrupt
volatile atomic_bool ev_signQuit = false; // If terminate signal happens, quit. Also, atomic variable

// Queue for holding clientFD
queue_t* g_queueRequest;

// Holds logfile FD
int logFileFD = STDERR_FILENO; // default logFile Descriptor

/*Function Declarations*/

/** @brief Checks if port given is valid
*   @param port:  Port num as 
*   @return port as a uint16_t
*/
uint16_t portCheck(char *port);


/** @brief
*/
void appropPlacer(char *buffer, long int bytesRead, char *requestLine, int *currentReqPos,
    bool *endRequest, long int *buffPosition, bool *prevR_1, bool *prevN_1, bool *prevR_2);

void URI_LOCK_INIT(int numThreads);

void FreeUriLocks();

void FinThread(int numThreads);

void FinThreadFree();

void* Worker_Request(void* arg);

/*Singal Interrupt*/
void sig_Interrupt(int signum) {
    printf("Start Signal Handling\n");
    if (signum == SIGTERM) {
        ev_s ign = true; // set sign quit to true
        condition_push(g_queueRequest); // Do the Condition Push
    }
}

int main(int argc, char *argv[]) {

    /* ==============setting up interrupt struct===================*/
    struct sigaction interrup_sign;
    memset(&interrup_sign, 0, sizeof(interrup_sign));
    interrup_sign.sa_handler = sig_Interrupt;
    sigaction(SIGTERM, &interrup_sign, NULL);

    /* --------------------- Checking for Arguments -----------------------------------------------------*/
    int char_get = 0; // Holds char value
    char log_file[1000]; // Holds log file file

    /*Flags for optional arguments*/
    bool l_flag = false;
    bool t_flag = false;
    int numThreads = 4; // default # of threads
    // If there are no arguments
    if (argc < 2) {
        warnx("Too few arguments:\nusage: ./httpserver [-t threads] [-l logfile] <port>");
        exit(1);
    }
    opterr = 0; // setting getopt print error to 0

    /*-------------------CHECKING FOR OPTION ARGUMENTS-------------------------------------------------*/
    while ((char_get = getopt(argc, argv, ":t:l:")) != -1) {
        switch (char_get) {
           
            /*Checking for number of threads*/
        case 'l':
            l_flag = true;
            // If not string after
            if (optarg != NULL) {
                strcpy(log_file, optarg);// copies string into log_file
            }
            else {
                warnx("invalid option input:\nusage: ./httpserver [-t threads] [-l logfile] <port>");
                exit(1);
            }
            break;

            /*Checking for logfile*/
        case 't':
            t_flag = true;
            if (optarg != NULL) {
                // checking if valid numbers:
                for (uint32_t i = 0; i < strlen(optarg); ++i) {
                    if ((optarg[i] < '0') || (optarg[i] > '9')) {
                        // need to exit program
                        warnx("invalid option input: ./httpserver [-t threads] [-l logfile] <port>");
                        exit(1);
                    }
                }
                numThreads = atoi(optarg);
            }
            else {
                warnx("invalid option input: ./httpserver [-t threads] [-l logfile] <port>");
                exit(1);
            }
            break;

            /*If some random option*/
        case '?':
            warnx("invalid option: ./httpserver [-t threads] [-l logfile] <port>");
            exit(1);

        case ':':
            warnx("No value given for option %c: ./httpserver [-t threads] [-l logfile] <port>", optopt);
            exit(1);
        }
    }
    /*===================================================================================================*/

    /*-------------------Getting port numbers---------------------------------------------------------*/
    if (optind == argc) { // if Invalid
        warnx("No port given: ./httpserver [-t threads] [-l logfile] <port>");
        exit(1);
    }
    /*====================================================================================================*/

    /*---------------------GETING PORT NUMBER AND CHECKING IF VALID--------------------*/
    uint16_t portNum = portCheck(argv[optind]); // Checking for valuid port number
    int socketFD = create_listen_socket(portNum);
    if (socketFD < 0) {
        warnx("bind: %s", strerror(errno));
        exit(1);
    }
    /*=================================================================================*/

    /*------------------------------Initializing queue for requests---------------------------------------*/
    g_queueRequest = queue_new(numThreads); // creates a queue size of threads
    /*====================================================================================================*/

    /*---------------------Opening Logfile if found---------------------*/
    if (l_flag) {
        // If the File Exists
        if (access(log_file, F_OK) == 0) { // Checking if file exists
            logFileFD = open(log_file, O_TRUNC | O_WRONLY); // getting logFileFD
            if (logFileFD < 0) // File error
            {
                warnx("logfile opener: %s", strerror(errno));
                exit(1);
            }
        }
        // If the file doesn't exitst
        else {
            logFileFD = open(log_file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
            if (logFileFD < 0) // File error
            {
                warnx("logfile opener: %s", strerror(errno));
                exit(1);
            }
        }
    }
    /*==================================================================*/
    
    /*Creat the URI locks*/
    URI_LOCK_INIT(numThreads); // creating URI locks

    /* Creating the Struct to hold whether a thread finished after a SIGTERM*/
    FinThread(numThreads);

    // creating the threads

    pthread_t worker_threads[numThreads]; // create numThreads workers

    // threads identifiers
    int threadNumber[numThreads];
    for (int i = 0; i < numThreads; ++i) {
        threadNumber[i] = i;
    }
    for (int i = 0; i < numThreads; ++i) {
        void* ThreadArg = &(threadNumber[i]);
        pthread_create(worker_threads + i, NULL, Worker_Request, ThreadArg);
    }

    //---------Processing client--------------------------------------------------------------------
    while (!ev_s ign) { // if the signal for quitting not set
        int* clientFD = malloc(sizeof(int));
        *(clientFD) = accept(socketFD, NULL, NULL); // waiting for connections

        /*If accept produces an error*/
        if ((*clientFD) < 0) {
            free(clientFD); // need to free clientFD memory
            if (errno == EINTR) { // need to break free and end program
                break;
            }
            else continue;  // Else try again
        }
        /*============================*/

        if (!queue_push(g_queueRequest, clientFD)) { // if clientFD not put in queue because sigterm
            free(clientFD);
        }
     }
    // going for array of threads
    bool allThreadFinish = false;

    /*Waits for all the threads to finish processing last requests*/
    printf("Handling closing threads here\n");
    while (!allThreadFinish) {
        int numFin = 0;// Shows how many threads finished
        for (int i = 0; i < g_signalThreads->size; ++i) {
            if (g_signalThreads->finishThreads[i]) {
                ++numFin; // Increment counter for finish
            }
            else {
                // Wakes up threads stuck in Queueing
                usleep(100);
                condition_pop(g_queueRequest); // Do condition Pop
            }
        }
        // Checking numFin
        if (numFin == g_signalThreads->size) allThreadFinish = true; // If all threads finish set to notify
    }
    for (int i = 0; i < numThreads; ++i) {
        pthread_join(worker_threads[i], NULL);
    }
    printf("End of Handling Requests\n");

    // Removing queue structs present
    while (queue_size(g_queueRequest) != 0) {
        void* strhere;
        queue_pop(g_queueRequest, &strhere);
        free(strhere);
    }
    queue_delete(&g_queueRequest);
    FreeUriLocks();
    FinThreadFree();
    fsync(logFileFD); //** Freeing memory for URI lock
    close(logFileFD); // Close log file
    return 0;
}


uint16_t portCheck(char *port) {
    for (unsigned int i = 0; port[i] != '\0'; ++i) {
        if (port[i] < '0' || port[i] > '9') {
            warnx("invalid port number: %s", port);
            exit(1);
        }
    }
    uint16_t result = strtol(port, NULL, 10);
    return result;
}


/*
* Pieces the request line together when the client is slow
*
*/
void appropPlacer(char *buffer, long int bytesRead, char *requestLine, int *currentReqPos,
    bool *endRequest, long int *buffPosition, bool *prevR_1, bool *prevN_1, bool *prevR_2) {
    // Used to check if reached end of request
    int i, j;
    for (j = *buffPosition, i = *currentReqPos; j < bytesRead; ++j, ++i) {
        char c = buffer[j]; // Gets char from buffer
        if ((c == '\r') && !(*prevR_1) && !(*prevN_1) && !(*prevR_2)) {
            *prevR_1 = true; // set the first \r to true
        } else if ((c == '\n') && *prevR_1 && !(*prevN_1) && !(*prevR_2)) {
            *prevN_1 = true;
        } else if ((c == '\r') && *prevR_1 && *prevN_1 && !(*prevR_2)) {
            *prevR_2 = true;
        } else if ((c == '\n') && *prevR_1 && *prevN_1 && *prevR_2) {
            // end of request so send a signal
            *endRequest = true; // End of request
            requestLine[i] = buffer[j];
            *buffPosition = ++j; // Set the buff position
            ++i;
            (*prevR_1) = (*prevN_1) = (*prevR_2) = false; // set everything to false
            break;
        } else {
            //set everything false
            (*prevR_1) = (*prevN_1) = (*prevR_2) = false;
        }
        requestLine[i] = buffer[j];
    }
    *currentReqPos = i;
}


/*This the function in which the worker threads will lay*/
void* Worker_Request(void* arg) {
    /*----------------Initializing all variables to process requesut----------------------*/
    // Need to read from the client
    int threadNum = *((int*)arg); // getting thread number
    long int bytesRead = 0;

    /* all variables needed*/
    char fileName[1000]; // Holds file name
    Methods Meth = NO_VALID;
    //bool ValidReq = true; // Shows if valid request

    /*Important HEADERS*/
    long int Content_length = 0; // Content_length
    long int Request_ID = 0; // REquest ID

    /*Boolean*/
    bool endRequest = false; // false if not the end of request
    bool p1 = false;
    bool p2 = false;
    bool p3 = false;
    bool clientQuit = false;
    /*Buffers Current Position*/
    int currentReqPos = 0;
    long int currentPosBuff = 0; // current position of buffer


    /*Buffers*/
    char requestLine[2048]; // Holds the request line
    char buffer[BUFF_SIZE + 1]; // Buffer size
    /*=========================================================================================*/



    // get client descriptor from queue
    void* getClient = NULL;
    int clientFD;
    while (!ev_s ign) { // Only exits if sign quit is set
        clientQuit = false;
        currentReqPos = 0;
        currentPosBuff = 0;
        Meth = NO_VALID;
        endRequest = false;
        p1 = p2 = p3 = false;

        if (!queue_pop(g_queueRequest, &getClient)) { // gets the client
            break;
        }
        clientFD = *((int*)getClient);
        free(getClient); // freeing the memory for the client 

        // need to set client to nonblocking
        fcntl(clientFD, F_SETFL, O_NONBLOCK);

        /*Read the client for the request*/
        while (!ev_s ign && !endRequest) {
            bytesRead = read(clientFD, buffer, BUFF_SIZE); // read the bytes
            if (bytesRead < 0) { // Checks for blocking
                if (errno == EAGAIN) {
                    continue;
                }
            }
            else if (bytesRead == 0) { // Client Closed connection
                clientQuit = true;
                break;
            }
            /*Process the read request*/
            else {
                buffer[bytesRead] = '\0';
                appropPlacer(buffer, bytesRead, requestLine, &currentReqPos, &endRequest,
                            &currentPosBuff, &p1, &p2, &p3);
            }
        }
        if (ev_s ign) {
            close(clientFD);
            break;
        }

        if (!clientQuit) { // if client Quit
        /*---------------------Checking for Response ----------------------------------*/
            int reqSize = currentReqPos;
            currentReqPos = 0;
            int result
                = request_format_RequestChecker(requestLine, &currentReqPos, reqSize, fileName, &Meth);
            if (result == -1) {
                // throw invalid request
                statusPrint(clientFD, BAD_REQUEST_); // print status
                close(clientFD);
                continue;
                
            }
            /*=============================================================================*/

            /*-------------------Checking the Header Fields---------------------------------*/
            result
                = request_format_HeaderFieldChecker(requestLine, &currentReqPos, reqSize, &Content_length, &Request_ID, &Meth);
            if (result == -1) {
                statusPrint(clientFD, BAD_REQUEST_);
                close(clientFD);
                continue;
            }
            // check if no valid Meth
            if (Meth == NO_VALID) {
                // Throw proper error
                statusPrint(clientFD, NOT_IMP_);
                close(clientFD);
                continue;
            }
            /*==============================================================================*/

            /* Checking for file existence*/
            int resultMeth = 0;
            /* Need to check if file is directory name*/
            //=============================================== PUT =================================================
            if (Meth == PUT) {
                resultMeth = PutReq(fileName, buffer, clientFD, &currentPosBuff,
                    &bytesRead, Content_length, Request_ID, logFileFD);

            }
            //================================================ GET =================================================
            else if (Meth == GET) {
                resultMeth = GetReq(fileName, clientFD, Request_ID, logFileFD);

                //=============================================== HEAD =================================================
            }
            else {
                resultMeth = HeadReq(fileName, clientFD, Request_ID, logFileFD);
            }
        }
        /*Resetting all variables to original values*/
        close(clientFD);
    }

    g_signalThreads->finishThreads[threadNum] = true;
    //condition_push(g_queueRequest); // Do the Condition Push
    return NULL;
}

/*URI locks that helps with the reader and write problem*/
void URI_LOCK_INIT(int numThreads) {
    g_uriLocks = malloc(sizeof(URI_lock));// creating array of locks for number of threads
    g_uriLocks->size = numThreads;
    sem_init(&(g_uriLocks->changeList), 1, 1); // Initializing lock to check array
    g_uriLocks->files = malloc(numThreads * sizeof(lock_file));
    for (int i = 0; i < numThreads; ++i) {
        (g_uriLocks->files)[i].URI[0] = '\0';
        (g_uriLocks->files)[i].numReaders = 0;
        (g_uriLocks->files)[i].numThreads = 0; // settign waiting/working threads to zero.
       sem_init(&((g_uriLocks->files)[i].fileWrite), 1, 1);
       sem_init(&((g_uriLocks->files)[i].numReadKey), 1, 1);
    }
}

void FreeUriLocks() {
    int size = g_uriLocks->size;
    for (int i = 0; i < size; ++i) {
        // unitialize all semaphores used
        sem_destroy(&((g_uriLocks->files)[i].fileWrite));
        sem_destroy(&((g_uriLocks->files)[i].numReadKey));
    }
    // now free the memory
    free(g_uriLocks->files);
    sem_destroy(&(g_uriLocks->changeList));
    free(g_uriLocks);
    g_uriLocks = NULL;
}

void FinThread(int numThreads) {
    g_signalThreads = malloc(sizeof(signalThreads)); // malloc array for singal threads
    g_signalThreads->size = numThreads; // Holds How many threads present
    g_signalThreads->finishThreads = calloc(numThreads, sizeof(atomic_bool)); // Initializing boolean array
}

void FinThreadFree() {
    free(g_signalThreads->finishThreads);// free bool array
    free(g_signalThreads);
    g_signalThreads = NULL;
}
