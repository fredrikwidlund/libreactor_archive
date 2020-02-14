CFLAGS = -std=gnu11 -march=native -O3 -flto -fuse-linker-plugin -Wall -Werror -Wpedantic
LDADD =-lreactor_core -ldynamic -lanl

http_client: http_client.o picohttpparser.o
	$(CC) -o $@ $< $(CFLAGS) $(LDADD)

tcp_client: tcp_client.o
	$(CC) -o $@ $< $(CFLAGS) $(LDADD)

resolver: resolver.o
	$(CC) -o $@ $< $(CFLAGS) $(LDADD)

rest_server: rest_server.o
	$(CC) -o $@ $< $(CFLAGS) $(LDADD)

http_server: http_server.o
	$(CC) -o $@ $< $(CFLAGS) $(LDADD)

tcp_server: tcp_server.o
	$(CC) -o $@ $< $(CFLAGS) $(LDADD)

clean:
	rm http_server tcp_server *.o
