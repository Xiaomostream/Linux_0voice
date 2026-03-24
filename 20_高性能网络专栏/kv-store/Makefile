
CC = gcc
TARGET = kvstore
SRCS = kvstore.c reactor.c Ntyco.c proactor.c Kvs_array.c Kvs_rbtree.c Kvs_hash.c
INC = -I ./NtyCo/core/
LIB = -L ./NtyCo/ -luring -lntyco
all:
	$(CC) -o $(TARGET) $(SRCS) $(INC) $(LIB)

clean:
	rm -rf kvstore
