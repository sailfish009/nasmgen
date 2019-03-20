CC = LC_ALL=C g++
CFLAGS += -Wall -Werror -g -O2
LFLAGS +=
NAME = nasmgen

SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.hpp)
OBJECTS = $(SOURCES:.cpp=.o)
PREFIX ?= /usr/local

$(NAME): $(OBJECTS)
	$(CC) \
		-o $(@) \
		$(^)

%.o: %.cpp $(wildcard *.hpp) Makefile
	$(CC) -c \
		$(CFLAGS) \
		$(<) \
		-o $(@)


%.asm: %.$(NAME) $(NAME)
	./$(NAME) \
		$(<) \
		> $(@)

%.asm.h: %.asm
	@echo "#pragma once" > $(@)
	@echo "extern \"C\" {" >> $(@)
	grep "^; extern " $(<) | sed 's/^; /    /' >> $(@)
	@echo "}" >> $(@)

%.asm.o: %.asm %.asm.h
	nasm -f elf64 \
		-o $(@) \
		$(<)

.PHONY: install
install: $(NAME)
	@install -v -t "$(DESTDIR)$(PREFIX)/bin" $(^)

clean:
	rm -f $(NAME) *.o *.asm *.asm.h
