# Makefile do projeto "so-unix-utils"
# - compila cada utilitário em src/ para bin/
# - coloca objetos em build/ para evitar recompilações desnecessárias

CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -D_POSIX_C_SOURCE=200809L
LDFLAGS ?=

# Estrutura de diretórios do projeto
SRC_DIR := src
INC_DIR := include
BIN_DIR := bin
OBJ_DIR := build

# Lista de programas a gerar (bin/<nome>)
PROGS := head tail cp mv rm grep sort ls islash

.PHONY: all clean

all: $(BIN_DIR) $(OBJ_DIR) $(addprefix $(BIN_DIR)/,$(PROGS))

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/common.o: $(SRC_DIR)/common.c $(INC_DIR)/common.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $(SRC_DIR)/common.c -o $(OBJ_DIR)/common.o

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/common.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(OBJ_DIR)/common.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)
