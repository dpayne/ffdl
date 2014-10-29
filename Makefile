NAME=ffdl
VERSION=0.1
EXECUTABLE_NAME= $(NAME)
LIB_NAME= lib$(NAME).dylib.$(VERSION)
STATIC_LIB_NAME= lib$(NAME).a

DIR=$(shell pwd)
DIR_SRC= $(DIR)/src
DIR_INCLUDE= $(DIR)/include
OBJECTS= $(DIR)/src/ffdl.o
LINKED_LIBS=curl

CC=gcc

# Folder to install build artifacts to
PREFIX=/usr/local

OS= $(shell uname)
CFLAGS= -fPIC -I$(DIR_INCLUDE) -I$(DIR_SRC) -O3 -fno-omit-frame-pointer -ffast-math -march=native -flto -Wall -Werror

ifeq ($(OS),Darwin)
	STATIC_LDFLAGS=-L./ -l$(NAME)
	LDFLAGS= -fPIC -lm -lcurl
else
	STATIC_LDFLAGS=-L./ -Wl,-Bstatic -l$(NAME)
	LDFLAGS= -fPIC -Wl,-Bdynamic -lm -lcurl
endif

.PHONY: all
all: $(OBJECTS) $(LIB_NAME) $(EXECUTABLE_NAME)

$(DIR_SRC)/%.o: %.c
	$(CC) $(LDFLAGS) $(CFLAGS) -c $< -o $@

$(LIB_NAME): $(OBJECTS)
	$(CC) -shared $(CFLAGS) -I$(DIR_INCLUDE) -I$(DIR_SRC) $(LDFLAGS) src/ffdl.c -o $(LIB_NAME);
	$(AR) -r $(STATIC_LIB_NAME) $(OBJECTS)

$(EXECUTABLE_NAME): $(LIB_NAME)
	$(CC) $(STATIC_LDFLAGS) $(CFLAGS) -I$(DIR_INCLUDE) -I$(DIR_SRC) $(LDFLAGS) main.c -o $(EXECUTABLE_NAME) $(OBJECTS);

.PHONY: install
install: $(EXECUTABLE_NAME) $(LIB_NAME)
	cp $(LIB_NAME) $(PREFIX)/lib
	cp $(STATIC_LIB_NAME) $(PREFIX)/lib
	cp $(EXECUTABLE_NAME) $(PREFIX)/bin

.PHONY: clean
clean:
	rm -f $(EXECUTABLE_NAME) $(LIB_NAME) $(STATIC_LIB_NAME) $(OBJECTS)
