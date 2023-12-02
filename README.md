# Simple Multithreaded Server in C by Fabert C.
## Cloning Repository
To clone the repository: `https://github.com/helghast098/Simple-C-Server.git`
## Repository Structure
The repository has the following file structure:

```bash
| README.md (This File)
├── 
│   ├── Bind  # C files to create a socket the server listens to.
│   │   ├── bind.c
│   │   └── bind.h
│   │
│   ├── Http_Server  # Main:  File initializes threads, parses command arguments, initializes locks for files, and a global var for when exit signal is received.
│   │   ├── httpserver.c
│   │   └── httpserver.h
│   │
│   ├── Http_methods  # C files where HTTP Methods (GET, HEAD, PUT) are processed, printing to server log file occurs, and locking and releasing of file locks happens.
│   │   ├── http_methods.c
│   │   └── http_methods.h
│   │
│   ├── Queue  # C files for a thread-safe queue that houses client file descriptors
│   │   ├── queue.c
│   │   └── queue.h
│   │
│   ├── Request_format  # C files for parsing requests from clients
│   │   ├── request_format.c
│   │   └── request_format.h
│   │
│   ├── LICENSE
│   ├── Makefile
└──
```
**How to run:**<br>
To run httpserver.c you must first type make on the Linux terminal, this will create a file named httpserver that can be run with ./httpserver ` [-t threads] [-l logfile] <port number>`. Note, `<port number>` must be greater than 1024 and be an integer.
If no optional arguments are provided, then the default value for threads is 4 and the logfile points to stdout.
    
    Example input to run httpserver: (No optional arguments)
        ./httpserver 8000
    What this command does is it sets up httpserver to listen on port 8000.

    Example input to run httpserver: (Optional Arguments)
        ./httpserver 8000 -t 5 -l logfile.txt 8000
    Here 5 threads are initialized, the requests to the server are written into logfile.txt, and the server listens on port 8000.

    Example PUT input from client using curl given port is 8000
        curl -X PUT localhost:8000/name.txt --data-raw "Fabert Charles"
    Creates file name.txt with contents "Fabert Charles"
        
