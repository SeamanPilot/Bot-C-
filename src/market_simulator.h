#pragma once
#include "stock.h"
#include <vector>
#include <map>
#include <memory>
#include <random>
#include <string>

// Simulates a stock market using Geometric Brownian Motion (GBM)
// for realistic price generation.
class MarketSimulator {
public:
    // drift: annualised expected return (e.g. 0.10 = 10%)
    // volatility: annualised std-dev    (e.g. 0.20 = 20%)
    // dt: fraction of year per bar      (e.g. 1/252 ≈ 1 trading day)
    explicit MarketSimulator(unsigned seed = 42);

    // Register a stock to be simulated
    void addStock(const std::string& symbol, const std::string& name,
                  double initial_price, double drift = 0.08,
                  double volatility = 0.25, double dt = 1.0 / 252.0);

    // Advance the simulation by one bar for all stocks
    void step();

    // Access a stock by symbol (nullptr if not found)
    Stock* getStock(const std::string& symbol);
    const std::vector<std::unique_ptr<Stock>>& stocks() const { return stocks_; }

    // Current prices map (symbol -> close price)
    std::map<std::string, double> currentPrices() const;

    int bar() const { return bar_; }

private:
    struct StockParams {
        double drift;
        double volatility;
        double dt;
    };

    std::vector<std::unique_ptr<Stock>> stocks_;
    std::map<std::string, StockParams>  params_;
    std::mt19937_64 rng_;
    std::normal_distribution<double> norm_;
    int bar_;
};
