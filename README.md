# Simple Multithreaded Server in C by Fabert C.
## Cloning Repository
To clone the repository: `https://github.com/helghast098/Simple-C-Server.git`
## Repository Structure
The repository has the following file structure:

```bash
| README.md (This File)
├── Simple-MultiThreaded-C-Server
│   │
│   ├── include # Header files
│   │   ├── Bind  # C files to create a socket the server listens to.
│   │   │   └── bind.h
│   │   │
│   │   ├── HTTP-Methods  # C files where HTTP Methods (GET, HEAD, PUT) are processed, printing to server log file occurs, and locking and releasing of file locks happens.
│   │   │   └── http_methods.h
│   │   │
│   │   ├── HTTP-Server  # Main:  File initializes threads, parses command arguments, initializes locks for files, and a global var for when exit signal is received.
│   │   │   └── httpserver.h
│   │   │
│   │   ├── Queue  # C files for a thread-safe queue that houses client file descriptors
│   │   │   └── queue.h
│   │   │
│   │   └── Request-Parser  # C files for parsing requests from clients
│   │       └── request_parser.h
│   │  
│   ├── src # HSource files
│   │   ├── Bind  # C files to create a socket the server listens to.
│   │   │   ├── bind.c
│   │   │   └── CMakeLists.txt
│   │   │
│   │   ├── HTTP-Methods  # C files where HTTP Methods (GET, HEAD, PUT) are processed, printing to server log file occurs, and locking and releasing of file locks happens.
│   │   │   ├── CMakeLists.txt
│   │   │   └── http_methods.c
│   │   │
│   │   ├── HTTP-Server  # Main:  File initializes threads, parses command arguments, initializes locks for files, and a global var for when exit signal is received.
│   │   │   ├── CMakeLists.txt
│   │   │   └── httpserver.c
│   │   │
│   │   ├── Queue  # C files for a thread-safe queue that houses client file descriptors
│   │   │   ├── CMakeLists.txt
│   │   │   └── queue.c
│   │   │
│   │   ├── Request-Parser  # C files for parsing requests from clients
│   │   │   ├── CMakeLists.txt
│   │   │   └── request_parser.c
│   │   │
│   │   └──  CMakeLists.txt
│   │
│   ├── CMakeLists.txt
│   ├── LICENSE
│   ├── Makefile
└──
```
**How to build:**<br>
`Using cmake:`
Create a directory named build in the main project directory.  On a terminal, navigate to build and type `cmake ..` then `make`.  This will create a directory named httpserver that contains an executable named httpserver.

`Using Makefile:`
Type ' make ' on a terminal that is open to this project.  This will create the server executable named httpserver.

**How to run:**<br>
You can run the server with `./httpserver  [-t threads] [-l logfile] <port number>`. Note, `<port number>` must be greater than 1024 and be an integer.
If no optional arguments are provided, then the default value for threads is 4 and the logfile points to stdout.

    Example input to run httpserver: (No optional arguments)
        ./httpserver 8000
    What this command does is it sets up httpserver to listen on port 8000.

    Example input to run httpserver: (Optional Arguments)
        ./httpserver 8000 -t 5 -l logfile.txt 8000
    Here 5 threads are initialized, the requests to the server are written into logfile.txt, and the server listens on port 8000.

    Example PUT input from client using curl given port is 8000
        curl -X PUT localhost:8000/name.txt --data-raw "Fabert Charles"
    Creates file name.txt with content "Fabert Charles"
