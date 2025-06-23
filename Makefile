TARGET = dns_proxy
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -D_DEFAULT_SOURCE
LDFLAGS =
SRCS = main.c
HEADERS = struct.h func.h
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)