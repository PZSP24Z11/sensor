# Define the compiler
CC = gcc

SRC = helpers.c dtls_server.c

HEADERS = constants.h helpers.h

OUT = dtls-server

WOLFSSL_FLAGS = $(shell wolfssl-config --cflags --libs)
CURL_FLAGS = -lcurl

all: $(OUT)

$(OUT): $(SRC) $(HEADERS)
	export LD_LIBRARY_PATH=/usr/local/lib:$$LD_LIBRARY_PATH && \
	$(CC) $(SRC) $(WOLFSSL_FLAGS) $(CURL_FLAGS) -o $(OUT)

run: all
	./$(OUT)

clean:
	rm -f $(OUT)