
# compile the program  
# Compiler
CC = clang

# Compiler flags
CFLAGS = -Wall -g 

# Source files
all : client nm ss1 ss2 ss3

client : client.c client.h
	$(CC) $(CFLAGS) client.c -o client

nm : namingserver.c trie.c requests.c lru.c log.c lru.h trie.h requests.h namingserver.h
	$(CC) $(CFLAGS) namingserver.c trie.c requests.c lru.c log.c -o nm

ss1: storageserver.c ss_functions.c lru.c lru.h ss_functions.h storageserver.h
	$(CC) $(CFLAGS) storageserver.c ss_functions.c lru.c -o ss1 -DPORT=8100

ss2: storageserver.c ss_functions.c lru.c lru.h ss_functions.h storageserver.h
	$(CC) $(CFLAGS) storageserver.c ss_functions.c lru.c -o ss2 -DPORT=8102

ss3: storageserver.c ss_functions.c lru.c lru.h ss_functions.h storageserver.h
	$(CC) $(CFLAGS) storageserver.c ss_functions.c lru.c -o ss3 -DPORT=8104

clean:
	rm -f client nm  ss1 ss2 ss3