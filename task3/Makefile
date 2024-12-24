# Указываем компилятор
CC = gcc

# Флаги компиляции
CFLAGS = -Wall -Wextra -I$(SRC_DIR) -I$(LLHTTP_DIR) -I$(VECTOR_DIR) -I$(HASHMAP_DIR)
LDFLAGS = $(LLHTTP_DIR)/libllhttp.a

# Директории
SRC_DIR = src
BIN_DIR = bin
LLHTTP_DIR = llhttp/build
VECTOR_DIR = c-vector
HASHMAP_DIR = hashmap.c

# Главный исполняемый файл
TARGET = $(BIN_DIR)/main

# Список исходных файлов
SRCS = $(wildcard $(SRC_DIR)/*.c $(VECTOR_DIR)/*.c $(HASHMAP_DIR)/*.c)
# Список объектных файлов
OBJS = $(SRCS:%.c=$(BIN_DIR)/%.o)

# Правило по умолчанию
all: $(TARGET)

# Правило сборки исполняемого файла
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Правило сборки объектных файлов
$(BIN_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка проекта
clean:
	rm -rf $(BIN_DIR)/*.o $(TARGET)

# Фиктивные цели
.PHONY: all clean

