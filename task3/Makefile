# Определяем ОС (для информации)
OS = Arch Linux

# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -march=native -I./llhttp/build # Добавляем путь к llhttp.h
LDFLAGS = -L./llhttp/build -lllhttp                         # Указываем путь к libllhttp.a и ссылку на библиотеку

# Папки
SRC_DIR = src
BIN_DIR = bin

# Название выходного файла
TARGET = $(BIN_DIR)/main

# Список исходных файлов
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Список объектных файлов
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

# Правило по умолчанию
all: $(TARGET)
	@echo "Building on $(OS)..."

# Сборка исполняемого файла
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Компиляция объектных файлов
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -rf $(BIN_DIR)

# Пересборка
rebuild: clean all
