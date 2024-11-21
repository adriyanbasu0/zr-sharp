CC = gcc
CFLAGS = -Wall -Wextra -I.
SRCS = main.c lexer.c parser.c interpreter.c
OBJS = $(SRCS:.c=.o)
TARGET = compiler

.PHONY: all clean install uninstall test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) test example a.out

install:
	@echo "Installing ZR#..."
	@mkdir -p $(HOME)/.local/bin
	@cp z $(TARGET) $(HOME)/.local/bin/
	@echo "Add $(HOME)/.local/bin to your PATH if not already added"

uninstall:
	@echo "Uninstalling ZR#..."
	@rm -f $(HOME)/.local/bin/z $(HOME)/.local/bin/$(TARGET)

test: all
	@echo "Running tests..."
	./z test.zr -o test
	./test
