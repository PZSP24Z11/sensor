# Define the compiler
CC = gcc

SRC = helpers.c dtls_server.c

TEST_SRC = tests/test_helpers.c helpers.c

HEADERS = constants.h helpers.h

OUT = dtls-server
TEST_OUT = test

WOLFSSL_FLAGS = $(shell wolfssl-config --cflags --libs)
CURL_FLAGS = -lcurl
CHECK_FLAGS = -lcheck -lm -lsubunit

all: $(OUT)

$(OUT): $(SRC) $(HEADERS)
	$(CC) $(SRC) $(WOLFSSL_FLAGS) $(CURL_FLAGS) -o $(OUT)

run: all
	export LD_LIBRARY_PATH=/usr/local/lib:$$LD_LIBRARY_PATH && \
	./$(OUT)

test: $(TEST_OUT)

$(TEST_OUT): $(TEST_SRC) $(HEADERS)
	$(CC) $(TEST_SRC) $(WOLFSSL_FLAGS) $(CURL_FLAGS) $(CHECK_FLAGS) -o $(TEST_OUT)

run_tests: test
	./$(TEST_OUT)

clean:
	rm -f $(OUT) $(TEST_OUT)
