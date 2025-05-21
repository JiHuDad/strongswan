CC=gcc
CFLAGS=-Wall -O2 -I/usr/include/cjson
LDFLAGS=-lcjson
TARGET=extsock_client
SRC=extsock_client.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) 