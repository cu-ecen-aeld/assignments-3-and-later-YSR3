CC ?= gcc                 
CFLAGS := -Wall -Werror -g
TARGET := aesdsocket
OBJS := aesdsocket.o

.PHONY: all default clean

all: $(TARGET)
default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o
