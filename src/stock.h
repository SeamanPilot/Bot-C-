#pragma once
#include <string>
#include <vector>
#include <deque>
#include <ctime>

// OHLCV bar: Open, High, Low, Close, Volume
struct Bar {
    double open;
    double high;
    double low;
    double close;
    double volume;
    std::time_t timestamp;
};

class Stock {
public:
    Stock(const std::string& symbol, const std::string& name, double initial_price);

    const std::string& symbol() const { return symbol_; }
    const std::string& name()   const { return name_; }
    double price() const;

    void addBar(const Bar& bar);
    const std::deque<Bar>& history() const { return history_; }
    size_t historySize() const { return history_.size(); }

    // Returns closing prices for the last `n` bars (most recent last)
    std::vector<double> closingPrices(size_t n = 0) const;

private:
    std::string symbol_;
    std::string name_;
    std::deque<Bar> history_;

    static const size_t MAX_HISTORY = 500;
};
