httpserver: httpserver.o bind.o httpmethod.o requestformat.o queue.o
	clang -Wall -Werror -Wextra -pedantic -lpthread -o httpserver httpserver.o bind.o httpmethod.o requestformat.o queue.o

httpserver.o: Http_Server/httpserver.c Http_Server/httpserver.h Bind/bind.h Http_methods/http_methods.h Request_format/request_format.h
	clang -Wall -Werror -Wextra -pedantic -c Http_Server/httpserver.c

httpmethod.o: Http_methods/http_methods.c Http_methods/http_methods.h
	clang -Wall -Werror -Wextra -pedantic -c -o httpmethod.o Http_methods/http_methods.c

requestformat.o: Request_format/request_format.c Request_format/request_format.h
	clang -Wall -Werror -Wextra -pedantic -c -o requestformat.o Request_format/request_format.c
	
bind.o: Bind/bind.c Bind/bind.h
	clang -Wall -Werror -Wextra -pedantic -c Bind/bind.c

queue.o: Queue/queue.c Queue/queue.h
	clang -Wall -Werror -Wextra -pedantic -c -o queue.o Queue/queue.c
	
clean:
	rm -f httpserver bind.o requestformat.o httpmethod.o httpserver.o queue.o
