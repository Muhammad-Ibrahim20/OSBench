CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -Iinclude -pthread

# Auto-detect OS
ifeq ($(OS),Windows_NT)
    TARGET = os_project.exe
    SRCS = src/main.cpp src/benchmark.cpp src/kalman_filter.cpp src/windows/syscalls_windows.cpp src/windows/kalman_windows.cpp
    LIBS = -lpsapi -ladvapi32
    RM = del /Q
else
    TARGET = os_project
    SRCS = src/main.cpp src/benchmark.cpp src/kalman_filter.cpp src/linux/syscalls_linux.cpp src/linux/kalman_linux.cpp
    LIBS = -pthread -lrt
    RM = rm -f
endif

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET) *.tmp *.dat kalman_*.csv sensor_*.csv benchmark_*.csv

.PHONY: all clean

