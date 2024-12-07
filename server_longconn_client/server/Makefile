VERSION = 1.0.0
CC = clang
DEBUG =
CFLAGS = -Wall
SOURCES = $(wildcard ./*.c ../../common/*.c)
INCLUDES = -I../../common
LIB_PATH = 
LIB_NAMES =
OBJ = $(patsubst %.c, %.o, $(SOURCES))
TARGET = server

$(TARGET):$(OBJ)
	@mkdir -p output
	$(CC) $(OBJ) $(LIB_PATH) $(LIB_NAMES) -o output/$(TARGET).$(VERSION)
	@rm -rf $(OBJ)
 
%.o: %.c
	$(CC) $(INCLUDES) $(DEBUG) -c $(CFLAGS) $< -o $@

.PHONY:clean
clean:
	rm -rf $(OBJ) $(TARGET) output 
