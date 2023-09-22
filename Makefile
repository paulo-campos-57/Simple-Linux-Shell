all: main.c
	gcc main.c -g -o shell -pthread
	
clean:
	rm shell