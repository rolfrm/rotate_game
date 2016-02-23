OPT = -g3 -O0
LIB_SOURCES = main.c game_data.c level_loader.c ../iron/mem.c ../iron/array.c ../iron/math.c ../iron/log.c ../iron/fileio.c
CC = gcc
TARGET = glitch
LIB_OBJECTS =$(LIB_SOURCES:.c=.o)
LDFLAGS= -L. -L../Corange $(OPT) -Wextra #-lmcheck #-ftlo #setrlimit on linux 
LIBS= -ldl  -lcorange -lGL -lSDL2 -lSDL2_net -lSDL2_mixer -lm
ALL= $(TARGET)
CFLAGS = -I.. -I../Corange/include -std=c11 -c $(OPT) -Wall -Wextra -Werror=implicit-function-declaration -Wformat=0 -D_GNU_SOURCE -fdiagnostics-color -Wextra  -Wwrite-strings -fbounds-check -Werror   #-DDEBUG 

$(TARGET): $(LIB_OBJECTS)
	$(CC) $(LDFLAGS) $(LIB_OBJECTS) $(LIBS) -o $@

all: $(ALL)

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@ -MMD -MF $@.depends 
depend: h-depend
clean:
	rm -f $(LIB_OBJECTS) $(ALL) *.o.depends

-include $(LIB_OBJECTS:.o=.o.depends)

