# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -Wall -g -std=c++17 -lpthread

# 目标文件
TARGETS = client server

# 对象文件
CLIENT_OBJ = client.o keysupplypacket.o packetbase.o
SERVER_OBJ = server.o keysupplypacket.o packetbase.o

# 生成所有目标
all: $(TARGETS)

# 链接 client
client: $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(CLIENT_OBJ)

# 链接 server
server: $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(SERVER_OBJ)

# 生成 client.o
client.o: client.cpp keysupplypacket.hpp packetbase.hpp
	$(CXX) $(CXXFLAGS) -c client.cpp

# 生成 server.o
server.o: server.cpp keysupplypacket.hpp packetbase.hpp
	$(CXX) $(CXXFLAGS) -c server.cpp

# 生成 keysupplypacket.o
keysupplypacket.o: keysupplypacket.cpp keysupplypacket.hpp packetbase.hpp
	$(CXX) $(CXXFLAGS) -c keysupplypacket.cpp

# 生成 packetbase.o
packetbase.o: packetbase.cpp packetbase.hpp
	$(CXX) $(CXXFLAGS) -c packetbase.cpp

# 清理编译生成的文件
clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean