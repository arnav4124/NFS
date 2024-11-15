# compile the program

# Compiler
CC = clang

# Compiler flags
CFLAGS = -Wall -std=c99 -g

# Source files
all : client nm ss

client : client.c client.h 
	$(CC) $(CFLAGS) client.c -o client

nm : namingserver.c trie.c requests.c lru.c lru.h trie.h requests.h namingserver.h
	$(CC) $(CFLAGS) namingserver.c trie.c requests.c lru.c -o nm

ss: storageserver.c ss_functions.c lru.c lru.h ss_functions.h storageserver.h
	$(CC) $(CFLAGS) storageserver.c ss_functions.c lru.c -o ss

clean: 
	rm -f client nm ss
