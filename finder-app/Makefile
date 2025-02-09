CC = $(CROSS_COMPILE)gcc

# Default target
all: writer

# Compile writer
writer: writer.o
	$(CC) -g -Wall writer.o -o writer

# Compile object file
writer.o: writer.c
	$(CC) -g -Wall -c writer.c -o writer.o

# Clean up
clean:
	rm -f writer.o writer
