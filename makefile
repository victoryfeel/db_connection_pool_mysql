CXX = g++
BUILD_TYPE = release

CXXFLAGS = -std=c++17 -Wall -I/usr/include/mysql -I/usr/include/mysql/mysql

ifeq ($(BUILD_TYPE),release)
CXXFLAGS += -O2
else
CXXFLAGS += -O0 -g
endif

LIBS = -L/usr/lib/ -lmariadb -lpthread

BIN_DIR = ./bin
SRC_DIR = ./src

COMMON_SRCS = $(SRC_DIR)/Connection.cpp $(SRC_DIR)/CommonConnectionPool.cpp
TARGET_POOL = $(BIN_DIR)/test_with_pool
TARGET_NO_POOL = $(BIN_DIR)/test_no_pool

all: $(TARGET_POOL) $(TARGET_NO_POOL)

$(TARGET_POOL): $(SRC_DIR)/test_with_pool.cpp $(COMMON_SRCS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

$(TARGET_NO_POOL): $(SRC_DIR)/test_no_pool.cpp $(COMMON_SRCS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)


clean:
	rm -rf $(BIN_DIR)
runp: $(TARGET_POOL)
	$(TARGET_POOL)
runn: $(TARGET_NO_POOL)
	$(TARGET_NO_POOL)
