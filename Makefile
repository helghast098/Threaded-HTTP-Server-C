

httpserver: httpserver.o bind.o httpmethod.o requestformat.o queue.o
	clang -Wall -Werror -Wextra -pedantic -lpthread -o httpserver httpserver.o bind.o httpmethod.o requestformat.o queue.o

httpserver.o: src/HTTP-Server/httpserver.c include/HTTP-Methods/http_methods.h include/HTTP-Server/httpserver.h include/Bind/bind.h include/Request-Parser/request_parser.h
	clang -Wall -Werror -Wextra -pedantic -Iinclude -c src/HTTP-Server/httpserver.c

httpmethod.o: src/HTTP-Methods/http_methods.c include/HTTP-Methods/http_methods.h
	clang -Wall -Werror -Wextra -pedantic -Iinclude -c -o httpmethod.o src/HTTP-Methods/http_methods.c

requestformat.o: src/Request-Parser/request_parser.c include/Request-Parser/request_parser.h
	clang -Wall -Werror -Wextra -pedantic -Iinclude  -c -o requestformat.o src/Request-Parser/request_parser.c

bind.o: src/Bind/bind.c include/Bind/bind.h
	clang -Wall -Werror -Wextra -pedantic -Iinclude -c -o bind.o src/Bind/bind.c

queue.o: src/Queue/queue.c include/Queue/queue.h
	clang -Wall -Werror -Wextra -pedantic -c -Iinclude -o queue.o src/Queue/queue.c

clean:
	rm -f httpserver bind.o requestformat.o httpmethod.o httpserver.o queue.o
