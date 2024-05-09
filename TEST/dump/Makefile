CC = gcc
CFLAGS = -Wall -Wextra -O3

CXX = g++
CXXFLAGS = -Wall -Wextra -O3

SRC_DIR = src
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(filter %.c, $(SRCS)))
OBJS += $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(filter %.cpp, $(SRCS)))

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

page: $(BUILD_DIR)/page.o
	$(CC)  $^ -o $(BUILD_DIR)/$@  $(CFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC)  $< -o $@ -c $(CFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $< -o $@ -c $(CXXFLAGS)

.PHONY: clean

clean: 
	rm -rf $(BUILD_DIR)