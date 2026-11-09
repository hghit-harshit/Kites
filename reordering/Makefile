CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

SRCS := parser.cpp DAG.cpp tester.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := tester

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp parser.h DAG.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean