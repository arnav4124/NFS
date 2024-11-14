# compile the program

# Compiler
CC = clang

# Compiler flags
CFLAGS = -Wall -std=c99 -g

# Source files
all : client nm ss

client : client.c 
	$(CC) $(CFLAGS) client.c -o client

nm : namingserver.c trie.c requests.c
	$(CC) $(CFLAGS) namingserver.c trie.c requests.c -o nm

ss: storageserver.c ss_functions.c
	$(CC) $(CFLAGS) storageserver.c ss_functions.c -o ss

clean: 
	rm -f client nm ss
