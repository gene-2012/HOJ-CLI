# 编译器设置
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -I.

# 目录定义
OBJ_DIR  := build
DIST_DIR := dist

# 目标程序名称及路径
TARGET_NAME := hoj-cli
ifeq ($(OS),Windows_NT)
    TARGET := $(DIST_DIR)/$(TARGET_NAME).exe
    # Windows 下创建目录和删除的兼容处理
    MKDIR  := if not exist $(OBJ_DIR) mkdir $(OBJ_DIR) && if not exist $(DIST_DIR) mkdir $(DIST_DIR)
    RM     := del /Q /S
    FIX_PATH = $(subst /,\,$1)
else
    TARGET := $(DIST_DIR)/$(TARGET_NAME)
    MKDIR  := mkdir -p $(OBJ_DIR) $(DIST_DIR)
    RM     := rm -rf
    FIX_PATH = $1
endif

# 源文件和对象文件
SRCS     := main.cpp
# 将 main.o 映射到 build/main.o
OBJS     := $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))

# 库链接设置
LIBS     := -lpthread
ifeq ($(OS),Windows_NT)
    LIBS += -lws2_32 -lshlwapi
endif

# 默认目标
all: $(TARGET)

# 链接阶段：将 build/*.o 链接成 dist/hoj-cli
$(TARGET): $(OBJS)
	@$(MKDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
	@echo  --- Build Success: $@ ---

# 编译阶段：将 .cpp 编译为 build/*.o
$(OBJ_DIR)/%.o: %.cpp
	@$(MKDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	$(RM) $(call FIX_PATH,$(OBJ_DIR))
	$(RM) $(call FIX_PATH,$(DIST_DIR))

.PHONY: all clean