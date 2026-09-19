CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic
TARGET   := search_engine
SRC      := main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET) docs

clean:
	rm -f $(TARGET)

.PHONY: all run clean
