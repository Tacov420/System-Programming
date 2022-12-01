all: server.c
	gcc server.c -D WRITE_SERVER -o write_server
	gcc server.c -D READ_SERVER -o read_server
clean:
	rm -f write_server.exe
	rm -f read_server.exe