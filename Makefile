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

#CFLAGS= -fPIC -I$(DIR_INCLUDE) -I$(DIR_SRC) -O3 -fno-omit-frame-pointer -ffast-math -march=native -flto -Wall -Werror -Wl,-Bdynamic -lm -lcurl
CFLAGS= -fPIC -I$(DIR_INCLUDE) -I$(DIR_SRC) -g3 -ggdb -Wall -Werror -Wl,-Bdynamic -lm -lcurl

.PHONY: all
all: $(OBJECTS) $(LIB_NAME) $(EXECUTABLE_NAME)

$(DIR_SRC)/%.o: %.c
	$(CC) $(LDFLAGS) $(CFLAGS) -c $< -o $@

$(LIB_NAME): $(OBJECTS)
	$(CC) -shared $(CFLAGS) -I$(DIR_INCLUDE) -I$(DIR_SRC) $(LDFLAGS) src/ffdl.c -o $(LIB_NAME);
	$(AR) -r $(STATIC_LIB_NAME) $(OBJECTS)

$(EXECUTABLE_NAME): $(LIB_NAME)
	$(CC) -L./ -Wl,-Bstatic -l$(NAME) $(CFLAGS) -I$(DIR_INCLUDE) -I$(DIR_SRC) $(LDFLAGS) main.c -o $(EXECUTABLE_NAME) $(OBJECTS);

.PHONY: install
install: $(EXECUTABLE_NAME) $(LIB_NAME)
	cp $(LIB_NAME) $(PREFIX)/lib
	cp $(STATIC_LIB_NAME) $(PREFIX)/lib
	cp $(EXECUTABLE_NAME) $(PREFIX)/bin

.PHONY: clean
clean:
	rm -f $(EXECUTABLE_NAME) $(LIB_NAME) $(STATIC_LIB_NAME) $(OBJECTS)
