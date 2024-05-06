CC = gcc
CFLAGS = -Wall -Wextra -O3
LDFLAGS = -lz

SRC_DIR = src
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

main: $(BUILD_DIR)/load.o $(BUILD_DIR)/main.o 
	$(CC)  $^ -o $(BUILD_DIR)/$@ $(LDFLAGS) $(CFLAGS)

run: $(BUILD_DIR)/load.o $(BUILD_DIR)/run.o
	$(CC)  $^ -o $(BUILD_DIR)/$@ $(LDFLAGS) $(CFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC)  $< -o $@ -c $(CFLAGS)

.PHONY: clean

clean: 
	rm -rf $(BUILD_DIR)