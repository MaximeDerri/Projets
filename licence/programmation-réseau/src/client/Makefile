.SILENT: all clean
SRC	= src
INC = include
EXEC	= client
CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -ggdb3 -I $(INC)

all: clean $(EXEC)
	
$(EXEC): 
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)/client.c $(SRC)/game_infos.c $(SRC)/command.c $(SRC)/io_tools.c $(SRC)/txt_channel.c

clean:
	rm client || true