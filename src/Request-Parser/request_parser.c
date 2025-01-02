/**
* Created by Fabert Charles
*
* Houses function definitions for request_format
*/

/*Libraries Included*/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "Request-Parser/request_parser.h"

typedef enum {
    STATE_START,
    STATE_FAILURE,
    STATE_START_KEY,
    STATE_KEY,
    STATE_START_VAL,
    STATE_VAL,
    STATE_1_R,
    STATE_1_N,
    STATE_2_R
} Header_State;

/*Function Definitions*/
int request_format_RequestChecker(
    char *buffer, int *currentPos, int bufferSize, char *file, Methods *meth) {
    int i = 0; // index of buffer
    if (bufferSize < 10)
        return -1;

    // Checking if Method is valid in buffer
    char method[10];
    for (; i <= METHOD_LENGTH + 1; ++i) {
        char c = buffer[i];

        // Valid Char Check
        if (((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c != ' '))
            || (i == METHOD_LENGTH + 1)) {
            return -1;
        }
        // Checking for spaces
        else {
            if (c == ' ') {
                method[i] = '\0';
                ++i;
                *currentPos = i;
                break;
            } else {
                method[i] = c;
            }
        }
    }

    if (strcmp("HEAD", method) == 0)
        *meth = HEAD; // HEAD
    else if (strcmp("GET", method) == 0)
        *meth = GET; // GET
    else if (strcmp("PUT", method) == 0)
        *meth = PUT; // PUT
    else
        *meth = NO_VALID; //If no valid input

    // Checking for Valid URI
    if (buffer[*currentPos] != '/')
        return -1;

    ++(*currentPos); // Increase current position to URI Start
    i = *currentPos;

    for (int j = 0; i < bufferSize; ++i, ++j) {
        char c = buffer[i];

        // checking if valid characters in file
        if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9') && (c != '.')
            && (c != '_') && (c != ' ') && (c != '/')) {
            return -1;
        }
        if (c == ' ') {
            file[j] = '\0';
            ++i;
            break;
        }
        file[j] = buffer[i];
    }

    *currentPos = i;

    // End if not enough length for version to be present in buffer
    if (*currentPos == bufferSize || (bufferSize - (*currentPos) < VERSION_LENGTH + 2)
        || buffer[i] != 'H')
        return -1;

    // Checking Version of HTTP
    char versionHolder[20]; // holds the version as string

    for (int j = 0, i = *currentPos; j < VERSION_LENGTH; ++j) {
        versionHolder[j] = buffer[i + j];
    }

    versionHolder[VERSION_LENGTH] = '\0';
    *currentPos += VERSION_LENGTH;

    // Comparing Version to correct version
    if (strcmp(versionHolder, "HTTP/1.1") != 0)
        return -1;

    if ((buffer[*currentPos] != '\r') || (buffer[(*currentPos) + 1] != '\n')) {
        return -1;
    } else {
        *currentPos += 2;
        return 0;
    }
}

int request_format_HeaderFieldChecker(int clientFD, char *buffer, int *currentPos, int bufferSize,
    long int *contLength, long int *reqID, Methods *method) {
    Header_State currentState = STATE_START;

    char key[1000];
    int key_indx = 0;

    char val[1000];
    int val_indx = 0;

    bool content_length_found = false;

    while (*currentPos != bufferSize) {
        switch (currentState) {
        case STATE_START:
            if (buffer[*currentPos] == '\r') {
                *currentPos += 1;
                currentState = STATE_2_R;
            } else if (buffer[*currentPos] == '\n') {
                return -1;
            } else {
                currentState = STATE_START_KEY;
            }
            break;

        case STATE_START_KEY:
            // remove all white spaces
            if (buffer[*currentPos] == ' ') {
                while (*currentPos != bufferSize && buffer[*currentPos] == ' ') {
                    *currentPos += 1;
                }
            } else if (buffer[*currentPos] == '\r' || buffer[*currentPos] == '\n'
                       || buffer[*currentPos] == ':') {
                return -1;
            } else {
                currentState = STATE_KEY;
            }
            break;

        case STATE_KEY:
            if (buffer[*currentPos] == ':') {
                key[key_indx] = '\0';
                *currentPos += 1;
                currentState = STATE_START_VAL;
            } else if (buffer[*currentPos] == ' ' || buffer[*currentPos] == '\r'
                       || buffer[*currentPos] == '\n') {
                return -1;
            } else {
                key[key_indx] = buffer[*currentPos];
                key_indx += 1;
                *currentPos += 1;
            }
            break;

        case STATE_START_VAL:
            // remove all white spaces
            if (buffer[*currentPos] == ' ') {
                while (*currentPos != bufferSize && buffer[*currentPos] == ' ') {
                    *currentPos += 1;
                }
            } else if (buffer[*currentPos] == ' ' || buffer[*currentPos] == '\r'
                       || buffer[*currentPos] == '\n') {
                return -1;
            } else {
                currentState = STATE_VAL;
            }
            break;

        case STATE_VAL:
            if (buffer[*currentPos] == '\r') {
                val[val_indx] = '\0';
                *currentPos += 1;
                currentState = STATE_1_R;
            } else if (buffer[*currentPos] == ' ' || buffer[*currentPos] == '\n') {
                return -1;
            } else {
                val[val_indx] = buffer[*currentPos];
                *currentPos += 1;
                val_indx += 1;
            }
            break;

        case STATE_1_R:
            if (buffer[*currentPos] != '\n') {
                return -1;
            }

            currentState = STATE_1_N;
            *currentPos += 1;
            break;

        case STATE_1_N:

            // Checking for key
            if (key_indx != 0) {
                // Content-Length
                if (strcmp(key, "Content-Length") == 0) {
                    *contLength = strtol(val, NULL, 10);
                    content_length_found = true;
                }
                // Request-Id
                else if (strcmp(key, "Request-Id") == 0) {
                    *reqID = strtol(val, NULL, 10);
                }
                // Expect
                else if (strcmp(key, "Expect") == 0) {
                    // Write now accepting any size file.
                    if (strcmp(val, "100-continue") == 0) {
                        if (write(clientFD, "HTTP/1.1 100 Continue\r\n\r\n", 29) < 0) {
                            return -1;
                        }
                    }
                }
            }

            if (buffer[*currentPos] != '\r') {
                currentState = STATE_START_KEY;
            } else {
                currentState = STATE_2_R;
                *currentPos += 1;
            }

            key_indx = 0;
            val_indx = 0;
            break;

        case STATE_2_R:
            if (buffer[*currentPos] != '\n') {
                return -1;
            }
            *currentPos += 1;

            if (*method == PUT && !content_length_found) {
                return -1;
            }
            return 0;
        }
    }

    return -1;
}
