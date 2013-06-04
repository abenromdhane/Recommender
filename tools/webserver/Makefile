CFLAGS := -I../../src -g
TARGETS := http-parser/http_parser.o post_parse.o server_parameters.o webserver.o ../../src/libRecommender.a libuv/uv.a
CC := gcc
all : webserver

webserver: $(TARGETS)
	gcc $(CFLAGS) $(TARGETS) -lrt -lm -o webserver

 ../../src/libRecommender.a:
	make -C  ../../src
libuv/uv.a:
	make -C libuv

http-parser/http_parser.o:
	make -C http-parser http_parser.o

webserver.o:
	$(CC) $(CFLAGS) -o webserver.o -c webserver.c

server_parameters.o:
	$(CC) $(CFLAGS) -o server_parameters.o -c server_parameters.c
post_parse.o:
	$(CC) $(CFLAGS) -o post_parse.o -c post_parse.c

clean:
	make -C http-parser clean
	make -C libuv distclean
	rm *.o
	rm webserver
