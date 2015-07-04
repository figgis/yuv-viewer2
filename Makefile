CC = gcc
#DBG        = -ggdb3
OPTFLAGS   = -Wstrict-prototypes -Wall -Wextra -Wpedantic $(DBG)  -O3
SDL_LIBS   := $(shell sdl2-config --libs)
SDL_CFLAGS := $(shell sdl2-config --cflags)
CFLAGS     = $(OPTFLAGS)  $(SDL_CFLAGS) -std=c99
LDFLAGS    = $(SDL_LIBS) #-lefence

SRC        = yv2.c
TARGET     = yv2
OBJ        = $(SRC:.c=.o)

default: $(TARGET)

%.o: %.c Makefile
	$(CC) $(CFLAGS)  -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm $(OBJ) $(TARGET)

