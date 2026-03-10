#pragma once
#include <string>
#include <vector>
#include <map>
#include <ctime>

// Order types
enum class OrderType { BUY, SELL };

// A completed trade
struct Trade {
    std::string symbol;
    OrderType   type;
    double      price;
    int         shares;
    double      commission;
    std::time_t timestamp;
};

// A current holding
struct Position {
    std::string symbol;
    int         shares;
    double      avg_cost;  // average cost per share
};

class Portfolio {
public:
    explicit Portfolio(double initial_cash, double commission_per_trade = 0.0);

    // Attempt to buy `shares` of `symbol` at `price`.
    // Returns true if executed, false if insufficient funds.
    bool buy(const std::string& symbol, double price, int shares);

    // Attempt to sell `shares` of `symbol` at `price`.
    // Returns true if executed, false if insufficient position.
    bool sell(const std::string& symbol, double price, int shares);

    double cash() const { return cash_; }

    // Total market value of all positions given current prices map
    double positionsValue(const std::map<std::string, double>& prices) const;

    // Total portfolio value (cash + positions)
    double totalValue(const std::map<std::string, double>& prices) const;

    // Unrealised P&L
    double unrealisedPnL(const std::map<std::string, double>& prices) const;

    // Realised P&L (sum of closed-trade profits)
    double realisedPnL() const { return realised_pnl_; }

    const std::map<std::string, Position>& positions() const { return positions_; }
    const std::vector<Trade>& tradeHistory() const { return trade_history_; }

    int sharesHeld(const std::string& symbol) const;

private:
    double cash_;
    double commission_;
    double realised_pnl_;
    std::map<std::string, Position> positions_;
    std::vector<Trade> trade_history_;
};
