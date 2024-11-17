# compile the program

# Compiler
CC = clang

define HASH
#
endef

# Compiler flags
CFLAGS = -Wall -std=c99 -g -Wno-$(HASH)warnings
 
# Source files
all : client nm 1ss 2ss

client : client.c client.h 
	$(CC) $(CFLAGS) client.c -o client

nm : namingserver.c trie.c requests.c lru.c lru.h trie.h requests.h namingserver.h
	$(CC) $(CFLAGS) namingserver.c trie.c requests.c lru.c -o nm

1ss: storageserver.c ss_functions.c lru.c lru.h ss_functions.h storageserver.h 
	$(CC) $(CFLAGS) storageserver.c ss_functions.c lru.c -o 1ss -DPORT=8086

2ss: storageserver.c ss_functions.c lru.c lru.h ss_functions.h storageserver.h
	$(CC) $(CFLAGS) storageserver.c ss_functions.c lru.c -o 2ss -DPORT=8088

clean: 
	rm -f client nm 1ss 2ss
