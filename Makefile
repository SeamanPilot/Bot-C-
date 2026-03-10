CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
SRCDIR   := src
SOURCES  := $(SRCDIR)/main.cpp \
            $(SRCDIR)/stock.cpp \
            $(SRCDIR)/portfolio.cpp \
            $(SRCDIR)/strategy.cpp \
            $(SRCDIR)/market_simulator.cpp
TARGET   := trading_bot

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
