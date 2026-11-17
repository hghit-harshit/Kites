# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2

# Target executable name
TARGET = tester

# Object files required to build the executable (using correct spelling: heurestic.o)
OBJS = tester.o parser.o DAG.o scheduler.o heurestic.o

# Default target
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compile individual source files
tester.o: tester.cpp parser.h DAG.h scheduler.h
	$(CXX) $(CXXFLAGS) -c tester.cpp

parser.o: parser.cpp parser.h
	$(CXX) $(CXXFLAGS) -c parser.cpp

DAG.o: DAG.cpp DAG.h
	$(CXX) $(CXXFLAGS) -c DAG.cpp

scheduler.o: scheduler.cpp scheduler.h
	$(CXX) $(CXXFLAGS) -c scheduler.cpp

heurestic.o: heurestic.cpp heurestic.h
	$(CXX) $(CXXFLAGS) -c heurestic.cpp

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean