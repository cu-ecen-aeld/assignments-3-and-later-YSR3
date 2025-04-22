CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g

TARGET = writer
OBJS = writer.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)


writer.o: writer.c
	$(CC) $(CFLAGS) -c writer.c


clean:
	rm -f $(TARGET) $(OBJS)
