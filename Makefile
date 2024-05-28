NAME_CLIENT=TrisClient
NAME_SERVER=TrisServer

CFLAGS=-Wall -std=gnu99 -m64
INCLUDES=-I./inc

SRCS=src/errExit.c src/TrisClient.c src/TrisServer.c src/semaphore.c src/utils.c

OBJ_DIR=obj
BIN_DIR=bin

OBJS=$(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRCS))

all: directories $(BIN_DIR)/$(NAME_CLIENT) $(BIN_DIR)/$(NAME_SERVER) $(BIN_DIR)/$(NAME_BOT)

$(BIN_DIR)/$(NAME_CLIENT): $(OBJ_DIR)/TrisClient.o $(OBJ_DIR)/errExit.o $(OBJ_DIR)/semaphore.o $(OBJ_DIR)/utils.o
	@echo "Making executable: "$@
	@$(CC) $^ -o $@

$(BIN_DIR)/$(NAME_SERVER): $(OBJ_DIR)/TrisServer.o $(OBJ_DIR)/errExit.o $(OBJ_DIR)/semaphore.o $(OBJ_DIR)/utils.o
	@echo "Making executable: "$@
	@$(CC) $^ -o $@

$(OBJ_DIR)/%.o: src/%.c
	@echo "Compiling: "$<
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean directories

clean:
	@echo "Cleaning up..."
	@$(RM) -r $(OBJ_DIR) $(BIN_DIR)

directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)
