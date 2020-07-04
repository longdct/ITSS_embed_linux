CC = gcc
CFLAG = -o
SERVER_FILES = server.c config.h message.h power_system.h power_supply.h connect_message.c connect_message.h power_supply_info_access.h utils.h equipment.h use_mode.h equipment.c message.c use_mode.c power_supply.c utils.c power_supply_info_access.c elec_power_ctrl.h log_write.h elec_power_ctrl.c log_write.c
CLIENT_FILES = client.c

all: server client

server: $(SERVER_FILES) 
	$(CC) $(CFLAG) server $(SERVER_FILES)

client: $(CLIENT_FILES)
	$(CC) $(CFLAG) client $(CLIENT_FILES)

clean: 
	$(RM) log/*