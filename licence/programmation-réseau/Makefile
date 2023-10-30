.SILENT: all server client clean new-client

SRC		= src/client/src
INC 	= -Isrc/client/include
EXEC	= client
CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -ggdb3 $(INC)

all: new-client server $(EXEC)

server:
	javac --source-path src/ -d build src/server/*.java
	
$(EXEC): 
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)/client.c $(SRC)/game_infos.c $(SRC)/command.c $(SRC)/io_tools.c $(SRC)/txt_channel.c

new-client:
	rm -f client

clean:
	rm -Rf build
	rm -Rf log/s*