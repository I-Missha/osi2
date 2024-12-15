# Указать компилятор
CC = gcc

# Флаги компиляции
CFLAGS = -Wall -Wextra -g

# Папки
SRC_DIR = src
BIN_DIR = bin

# Название выходного файла
TARGET = $(BIN_DIR)/main

# Список исходных файлов
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Список объектных файлов (замена .c на .o)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

# Правило по умолчанию
all: $(TARGET)

# Сборка исполняемого файла
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Компиляция объектных файлов
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -rf $(BIN_DIR)

# Пересборка
rebuild: clean all

