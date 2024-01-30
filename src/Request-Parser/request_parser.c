/**
* Created by Fabert Charles
*
* Houses function definitions for request_format
*/

/*Libraries Included*/
#include <stdlib.h>
#include <string.h>
#include <Request-Parser/request_parser.h>

/*Function Definitions*/
int request_format_RequestChecker(char* buffer, int* currentPos, int bufferSize, char* file, Methods* meth) {
    int i = 0; // index of buffer
    if (bufferSize < 10)
        return -1;

    // Checking if Method is valid in buffer
    char method[10];
    for (; i <= METHOD_LENGTH + 1; ++i) {
        char c = buffer[i];

        // Valid Char Check
        if (((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c != ' ')) || (i == METHOD_LENGTH + 1)) {
            return -1;
        }
        // Checking for spaces
        else {
            if (c == ' ') {
                method[i] = '\0';
                ++i;
                *currentPos = i;
                break;
            }
            else {
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
    if (*currentPos == bufferSize || (bufferSize - (*currentPos) < VERSION_LENGTH + 2) || buffer[i] != 'H')
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
    }
    else {
        *currentPos += 2;
        return 0;
    }
}


int request_format_HeaderFieldChecker(char* buffer, int* currentPos, int bufferSize, long int* contLength, long int* reqID, Methods* method) {
    // Checks if there are no header
    if ((bufferSize - *currentPos) < 2) {
        *contLength = 0;
        return -1;
    }

    // Checking if beginning end with \r and \n
    if (buffer[*currentPos] == '\r' && buffer[*currentPos + 1] == '\n') {
        // add check if method is put
        *contLength = 0;
        if (*method == PUT)
            return -1;
        return 0;
    }

    char key[1000]; // Key of Header Field
    char val[1000]; // Value of Header Field
    bool contentLengthFound = false; // true if content length found
    bool contentForPut = false; // Only for put commands.  True if client wants to insert something
    bool keyFound = true; // True if a key is found
    bool endChecker = false; // True when no more header fields
    bool requestID = false;  // True if request ID heaeder field present
    *reqID = 0; // Just in case if request ID not found

    for (int j = 0, i = *currentPos; i < bufferSize; ++i, ++j) {
        char c = buffer[i]; // char of header fields

        // Checking if end of header fields
        if (endChecker) {
            // If c = '\r'
            if (c == '\r') {
                if ((bufferSize - i + 1) > 1) {
                    if (buffer[i + 1] == '\n') {
                        // Set the appropriate current Pos
                        *currentPos = i + 2;

                        // Checks if there is content length for put
                        if ((!contentForPut) && (*method == PUT)) {
                            *contLength = 0;
                            return -1;
                        }
                        return 0;
                    }
                    else {
                        endChecker = false;
                        keyFound = true;
                    }
                }
            }
            // No end char found so move to next key
            else {
                endChecker = false;
                keyFound = true;
            }
        }

        // Checking if Key Found
        if (keyFound) {
            // Checking for white space in front of key
            if (j == 0 && c == ' ') {
                // remove white spaces unitl reaching key
                for (; i < bufferSize; ++i) {
                    c = buffer[i];
                    if (c != ' ')
                        break; //If no white space encountered
                }
                if (i == bufferSize)
                    return -1;
            }

            // If white space is present
            if (c == ' ') {
                *contLength = 0;
                return -1;
            }
            // Transition to value
            else if (c == ':') {
                // Checks if there is actually a key
                if (j == 0) {
                    *currentPos = i;
                    *contLength = 0;
                    return -1;
                }

                keyFound = false; // Turn off finding key
                key[j] = '\0';

                // If there is more than 1 char to handle
                if ((bufferSize - i + 1) > 1) {
                    // Checks for space after ":"
                    if (buffer[i + 1] != ' ') {
                        *currentPos = i + 1;
                        *contLength = 0;
                        return -1;
                    }
                    else {
                        // IMPORTANT HEADER FIELDS
                        // Checking if content_length Found
                        if (strcmp(key, "Content-Length") == 0) {
                            contentLengthFound = true;
                            contentForPut = true;
                        }
                        // Checking for request Id
                        else if (strcmp(key, "Request-Id") == 0) {
                            requestID = true; // foudn request ID

                        }
                        // Now checking for value
                        ++i;
                        j = -1; // reseting j to 0
                        continue; // go to next loop iteration
                    }
                }
                else {
                    *currentPos = i;
                    *contLength = 0;
                    return -1;
                }
            }
            else {
                key[j] = c;
            }
        }
        // Finding Value of Key
        else {
            // Checks if after space there is the end headers delimiters
            // Only Find Values for Content length or Request ID
            if (contentLengthFound || requestID) {
                // remove white spaces from front of number
                if (j == 0 && c == ' ') {
                    // remove white spaces unitl reaching key
                    for (; i < bufferSize; ++i) {
                        c = buffer[i];

                        if (c != ' ')
                            break; //If no white space encountered
                    }

                    if (i == bufferSize)
                        return -1;
                }
                // If at the end of value
                if (c == '\r') {
                    val[j] = '\0';
                    // Check for empty string
                    if (strlen(val) == 0) {
                        *contLength = 0;
                        return -1;
                    }

                    // Checking if the next value is \n
                    if ((bufferSize - i + 1) > 1) {
                        if (buffer[i + 1] != '\n') {
                            *currentPos = i + 1;
                            *contLength = 0;
                            return -1;
                        }

                        // Storing value
                        if (contentLengthFound) *contLength = strtol(val, NULL, 10); // For Content_length
                        else *reqID = strtol(val, NULL, 10); // For Request ID

                        // Need to increment i and reset j
                        ++i;
                        j = -1;
                        endChecker = true;
                        requestID = false; // turn off requestID
                        contentLengthFound = false; // turn of contentHE
                        continue;
                    }
                }
                // If the char is not a integer
                if ((c < '0') || (c > '9')) {
                    *currentPos = i;
                    *contLength = 0;
                    return -1;
                }

                val[j] = c;
            }
            // Checking for other headers
            else {
                // End of value
                if (c == '\r') {
                    // NEED TO START END OF HEADER CHECKER AND ALSO CHECK IS NEXT CHAR IS \N
                    if ((bufferSize - i + 1) > 1) {
                        if (buffer[i + 1] == '\n') // found delimiter of key and value
                        {
                            ++i;
                            j = -1;
                            endChecker = true;
                        }
                        // Need to char end checker of header
                    }
                }
            }
        }
    }
    *currentPos = bufferSize;
    *contLength = 0;
    return -1;
}
