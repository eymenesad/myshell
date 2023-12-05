CC = gcc
CFLAGS = -Wall

myshell: myshell.c
	$(CC) $(CFLAGS) -o myshell myshell.c

clean:
	rm -f myshell
