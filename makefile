#*******************************************************************************
#* Name: Sanjay Athrey
#* Pledge: I pledge my honor that I have abided by the Stevens Honor System
#******************************************************************************/
CC     = gcc
C_FILE = chatclient.c
TARGET = chatclient
SERVER_FILE = chatserver.c
SERVER_TARGET = chatserver
CFLAGS = -Wall -Werror -pedantic-errors

all: chatclient.c
	$(CC) $(CFLAGS) $(C_FILE) -o $(TARGET)
server:
	$(CC) $(CFLAGS) $(SERVER_FILE) -o $(SERVER_TARGET)
clean:
	rm -f $(TARGET) $(TARGET).exe $(SERVER_TARGET) $(SERVER_TARGET).exe
