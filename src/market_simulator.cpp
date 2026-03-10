#include "market_simulator.h"
#include <cmath>
#include <stdexcept>

MarketSimulator::MarketSimulator(unsigned seed)
    : rng_(seed), norm_(0.0, 1.0), bar_(0) {}

void MarketSimulator::addStock(const std::string& symbol, const std::string& name,
                                double initial_price, double drift,
                                double volatility, double dt) {
    stocks_.push_back(std::make_unique<Stock>(symbol, name, initial_price));
    params_[symbol] = {drift, volatility, dt};
}

void MarketSimulator::step() {
    ++bar_;
    std::time_t ts = std::time(nullptr) + bar_ * 86400; // synthetic daily timestamp
    for (auto& stock : stocks_) {
        const auto& p = params_.at(stock->symbol());
        double prev_close = stock->price();
        // GBM: S(t+dt) = S(t) * exp((mu - 0.5*sigma^2)*dt + sigma*sqrt(dt)*Z)
        double drift_term  = (p.drift - 0.5 * p.volatility * p.volatility) * p.dt;
        double diff_term   = p.volatility * std::sqrt(p.dt) * norm_(rng_);
        double new_close   = prev_close * std::exp(drift_term + diff_term);

        // Generate intra-bar high/low with some noise
        double noise_hi = 1.0 + std::abs(norm_(rng_)) * p.volatility * std::sqrt(p.dt) * 0.5;
        double noise_lo = 1.0 - std::abs(norm_(rng_)) * p.volatility * std::sqrt(p.dt) * 0.5;
        double bar_high  = std::max(prev_close, new_close) * noise_hi;
        double bar_low   = std::min(prev_close, new_close) * std::max(noise_lo, 0.01);
        double volume    = 1000000.0 * (0.5 + std::abs(norm_(rng_)));

        Bar bar{prev_close, bar_high, bar_low, new_close, volume, ts};
        stock->addBar(bar);
    }
}

Stock* MarketSimulator::getStock(const std::string& symbol) {
    for (auto& s : stocks_)
        if (s->symbol() == symbol) return s.get();
    return nullptr;
}

std::map<std::string, double> MarketSimulator::currentPrices() const {
    std::map<std::string, double> prices;
    for (const auto& s : stocks_)
        prices[s->symbol()] = s->price();
    return prices;
}
