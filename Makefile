CC = gcc
CFLAG = 
SERVER_FILES = server.c config.h message.h power_system.h power_supply.h power_supply_info_access.h utils.h equipment.h use_mode.h equipment.c message.c use_mode.c power_supply.c utils.c power_supply_info_access.c 
CLIENT_FILES = client.c

all: server client

server: $(SERVER_FILES) 
	$(CC) $(CFLAG) -o server $(SERVER_FILES)

client: $(CLIENT_FILES)
	$(CC) $(CFLAG) -o client $(CLIENT_FILES)

clean: 
	$(RM) log/*