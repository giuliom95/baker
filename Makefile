# options
CXX=g++
CXXFLAGS=-std=c++11 -O3 -Wall -Wno-deprecated -msse3
TARGET=bin/baker

INCLUDES=

LDFLAGS=-pthread -lembree3

# globs
SRCS := $(wildcard src/*.cpp)
HDRS := $(wildcard src/*.hpp)
OBJS := $(patsubst src/%.cpp,bin/%.o,$(SRCS))

# link it all together
$(TARGET): $(OBJS) $(HDRS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) -o $(TARGET)

# compile an object based on source and headers
bin/%.o: src/%.cpp $(HDRS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# tidy up
clean:
	rm -f $(TARGET) $(OBJS)

all: $(TARGET)
	@echo "All done!"