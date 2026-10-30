CC = gcc
CFLAGS = -Iinclude
SRC = src
TESTS = tests
OBJ = build
BIN = bin

SRC_SRCS = $(wildcard $(SRC)/*.c)
TEST_SRCS = $(wildcard $(TESTS)/*.c)
SRCS = $(SRC_SRCS) $(TEST_SRCS)

SRC_OBJS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRC_SRCS))
TEST_OBJS = $(patsubst $(TESTS)/%.c, $(OBJ)/%.o, $(TEST_SRCS))
OBJS = $(SRC_OBJS) $(TEST_OBJS)

all: $(BIN)/jobExecutorServer $(BIN)/jobCommander $(BIN)/progDelay
# $(BIN)/test

$(BIN)/jobExecutorServer: $(OBJ)/jobExecutorServer.o $(OBJ)/DequeImplementation.o $(OBJ)/variables.o
	@mkdir -p $(BIN)
	$(CC) -o $@ $^ -lpthread

$(BIN)/jobCommander: $(OBJ)/jobCommander.o $(OBJ)/variables.o
	@mkdir -p $(BIN)
	$(CC) -o $@ $^

# $(BIN)/test: $(OBJ)/test.o
# 	@mkdir -p $(BIN)
# 	$(CC) -o $@ $^

$(BIN)/progDelay: $(OBJ)/progDelay.o
	@mkdir -p $(BIN)
	$(CC) -o $@ $^

$(OBJ)/jobExecutorServer.o: $(SRC)/jobExecutorServer.c $(OBJ)/DequeImplementation.o $(OBJ)/variables.o
	@mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/jobCommander.o: $(SRC)/jobCommander.c $(OBJ)/variables.o
	@mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/DequeImplementation.o: $(SRC)/DequeImplementation.c
	@mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/variables.o: $(SRC)/variables.c
	@mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

# $(OBJ)/test.o: $(TESTS)/test.c
# 	@mkdir -p $(OBJ)
# 	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/progDelay.o: $(TESTS)/progDelay.c
	@mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)/*.o $(BIN)/*
