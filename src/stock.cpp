#include "stock.h"
#include <stdexcept>

Stock::Stock(const std::string& symbol, const std::string& name, double initial_price)
    : symbol_(symbol), name_(name)
{
    if (initial_price <= 0.0)
        throw std::invalid_argument("Initial price must be positive");
    Bar bar{initial_price, initial_price, initial_price, initial_price, 0.0, std::time(nullptr)};
    history_.push_back(bar);
}

double Stock::price() const {
    return history_.empty() ? 0.0 : history_.back().close;
}

void Stock::addBar(const Bar& bar) {
    history_.push_back(bar);
    if (history_.size() > MAX_HISTORY)
        history_.pop_front();
}

std::vector<double> Stock::closingPrices(size_t n) const {
    size_t count = (n == 0 || n > history_.size()) ? history_.size() : n;
    std::vector<double> prices;
    prices.reserve(count);
    size_t start = history_.size() - count;
    for (size_t i = start; i < history_.size(); ++i)
        prices.push_back(history_[i].close);
    return prices;
}
