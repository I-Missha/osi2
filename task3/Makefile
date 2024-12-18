# Указываем компилятор
CC = gcc

# Флаги компиляции
CFLAGS = -Wall -Wextra -I$(SRC_DIR) -Illhttp/build
LDFLAGS = llhttp/build/libllhttp.a
# Директории
SRC_DIR = src
BIN_DIR = bin
LLHTTP_DIR = ../llhttp/build

# Главный исполняемый файл
TARGET = $(BIN_DIR)/main

# Список исходных файлов
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Список объектных файлов
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

# Правило по умолчанию
all: $(TARGET)

# Правило сборки исполняемого файла
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Правило сборки объектных файлов
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка проекта
clean:
	rm -rf $(BIN_DIR)/*.o $(TARGET)

# Фиктивные цели
.PHONY: all clean

