
# compile the program  
# Compiler
CC = clang

define HASH 
#
endef

# Compiler flags
CFLAGS = -Wall -g -Wno-$(HASH)warnings

# Source files
all : client nm  1ss 2ss 3ss

client : client.c client.h
	$(CC) $(CFLAGS) client.c -o client

nm : namingserver.c trie.c requests.c lru.c log.c lru.h trie.h requests.h namingserver.h
	$(CC) $(CFLAGS) namingserver.c trie.c requests.c lru.c log.c -o nm

1ss: storageserver.c ss_functions.c lru.c lru.h ss_functions.h storageserver.h
	$(CC) $(CFLAGS) storageserver.c ss_functions.c lru.c -o 1ss -DPORT=8081

2ss: storageserver.c ss_functions.c lru.c lru.h ss_functions.h storageserver.h
	$(CC) $(CFLAGS) storageserver.c ss_functions.c lru.c -o 2ss -DPORT=8082

3ss: storageserver.c ss_functions.c lru.c lru.h ss_functions.h storageserver.h
	$(CC) $(CFLAGS) storageserver.c ss_functions.c lru.c -o 3ss -DPORT=8083

clean:
	rm -f client nm  1ss 2ss 3ss