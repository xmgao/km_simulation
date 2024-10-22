CC = g++
CFLAGS = -std=c++17

# 目标文件
TARGETS = appproactive apppasive

# 公共源文件和头文件
COMMON_SOURCES = Encryptor.cpp keyrequestpacket.cpp opensessionpacket.cpp packetbase.cpp
COMMON_HEADERS = Encryptor.hpp keyrequestpacket.hpp opensessionpacket.hpp packetbase.hpp

# 特定的源文件
PROACTIVE_SRC = appproactive.cpp
PASSIVE_SRC = appppassive.cpp

# 特定的对象文件
PROACTIVE_OBJ = $(PROACTIVE_SRC:.cpp=.o)
PASSIVE_OBJ = $(PASSIVE_SRC:.cpp=.o)

all: $(TARGETS)

appproactive: $(PROACTIVE_OBJ) $(COMMON_SOURCES:.cpp=.o)
	$(CC) $^ -o $@ $(CFLAGS)

apppasive: $(PASSIVE_OBJ) $(COMMON_SOURCES:.cpp=.o)
	$(CC) $^ -o $@ $(CFLAGS)

# 通用的cpp文件编译规则
%.o: %.cpp $(COMMON_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean