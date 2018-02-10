OBJ2	= ContentServer.o
OBJ1	= MirrorInitiator.o
OBJ3	= MirrorServer.o
OUT1	= MirrorInitiator
OUT2	= ContentServer
OUT3	= MirrorServer
CC		= gcc
FLAGS	= -g -c -Wall

all: $(OUT2) $(OUT1) $(OUT3)
MirrorInitiator.o: MirrorInitiator.c
	$(CC) $(FLAGS) MirrorInitiator.c
ContentServer.o: ContentServer.c
	$(CC) $(FLAGS) ContentServer.c
MirrorServer.o: MirrorServer.c
	$(CC) $(FLAGS) MirrorServer.c
MirrorInitiator: $(OBJ1)
	$(CC) $(OBJ1) -o $(OUT1)
ContentServer: $(OBJ2)
	$(CC) $(OBJ2) -o $(OUT2)
MirrorServer: $(OBJ3)
	$(CC) -pthread $(OBJ3) -o $(OUT3)
# clean up
clean:
	rm -rf $(OBJ1) $(OBJ2) $(OBJ3) $(OUT1) $(OUT2) $(OUT3)
