NAME_CLIENT=TrisClient
NAME_SERVER=TrisServer
CFLAGS=-Wall -std=gnu99 -m64
INCLUDES=-I./inc

SRCS=$(wildcard src/*.c)
OBJ_DIR=obj
BIN_DIR=bin

OBJS=$(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRCS))

all: directories $(BIN_DIR)/$(NAME_CLIENT) $(BIN_DIR)/$(NAME_SERVER)

#Crea l'eseguibile TrisClient
$(BIN_DIR)/$(NAME_CLIENT): $(OBJS)
	@echo "Making executable: $@"
	@$(CC) $^ -o $@

#Crea l'eseguibile TrisServer
$(BIN_DIR)/$(NAME_SERVER): $(OBJS)
	@echo "Making executable: $@"
	@$(CC) $^ -o $@

$(OBJ_DIR)/%.o: src/%.c
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean directories

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Removed object files and executables directories..."

directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)
