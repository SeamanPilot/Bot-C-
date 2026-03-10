#include "portfolio.h"
#include <stdexcept>

Portfolio::Portfolio(double initial_cash, double commission_per_trade)
    : cash_(initial_cash), commission_(commission_per_trade), realised_pnl_(0.0) {
    if (initial_cash < 0.0)
        throw std::invalid_argument("Initial cash cannot be negative");
}

bool Portfolio::buy(const std::string& symbol, double price, int shares) {
    if (shares <= 0 || price <= 0.0) return false;
    double total_cost = price * shares + commission_;
    if (total_cost > cash_) return false;

    cash_ -= total_cost;
    auto& pos = positions_[symbol];
    if (pos.shares == 0) {
        pos.symbol   = symbol;
        pos.avg_cost = price;
        pos.shares   = shares;
    } else {
        double total_prev = pos.avg_cost * pos.shares;
        pos.shares   += shares;
        pos.avg_cost  = (total_prev + price * shares) / pos.shares;
    }
    trade_history_.push_back({symbol, OrderType::BUY, price, shares,
                               commission_, std::time(nullptr)});
    return true;
}

bool Portfolio::sell(const std::string& symbol, double price, int shares) {
    if (shares <= 0 || price <= 0.0) return false;
    auto it = positions_.find(symbol);
    if (it == positions_.end() || it->second.shares < shares) return false;

    auto& pos = it->second;
    double proceeds = price * shares - commission_;
    double cost     = pos.avg_cost * shares;
    realised_pnl_  += proceeds - cost;
    cash_          += proceeds;
    pos.shares     -= shares;
    if (pos.shares == 0)
        positions_.erase(it);

    trade_history_.push_back({symbol, OrderType::SELL, price, shares,
                               commission_, std::time(nullptr)});
    return true;
}

double Portfolio::positionsValue(const std::map<std::string, double>& prices) const {
    double total = 0.0;
    for (const auto& [sym, pos] : positions_) {
        auto it = prices.find(sym);
        if (it != prices.end())
            total += it->second * pos.shares;
    }
    return total;
}

double Portfolio::totalValue(const std::map<std::string, double>& prices) const {
    return cash_ + positionsValue(prices);
}

double Portfolio::unrealisedPnL(const std::map<std::string, double>& prices) const {
    double pnl = 0.0;
    for (const auto& [sym, pos] : positions_) {
        auto it = prices.find(sym);
        if (it != prices.end())
            pnl += (it->second - pos.avg_cost) * pos.shares;
    }
    return pnl;
}

int Portfolio::sharesHeld(const std::string& symbol) const {
    auto it = positions_.find(symbol);
    return it == positions_.end() ? 0 : it->second.shares;
}
