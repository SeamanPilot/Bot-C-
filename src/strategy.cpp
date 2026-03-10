#include "strategy.h"
#include "indicators.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

// ---------------------------------------------------------------------------
// Base Strategy
// ---------------------------------------------------------------------------
int Strategy::orderSize(const Stock& stock, const Portfolio& portfolio) const {
    double alloc = portfolio.cash() * DEFAULT_POSITION_FRACTION;
    double price = stock.price();
    if (price <= 0.0) return 0;
    int shares = static_cast<int>(alloc / price);
    return std::max(shares, 0);
}

// ---------------------------------------------------------------------------
// Moving Average Crossover
// ---------------------------------------------------------------------------
MACrossoverStrategy::MACrossoverStrategy(size_t short_period, size_t long_period)
    : short_period_(short_period), long_period_(long_period),
      prev_short_sma_(0.0), prev_long_sma_(0.0) {
    if (short_period >= long_period)
        throw std::invalid_argument("Short period must be less than long period");
}

Signal MACrossoverStrategy::evaluate(const Stock& stock, const Portfolio& /*portfolio*/) {
    auto prices = stock.closingPrices();
    if (prices.size() < long_period_) return Signal::HOLD;

    double cur_short = Indicators::sma(prices, short_period_);
    double cur_long  = Indicators::sma(prices, long_period_);

    Signal sig = Signal::HOLD;
    if (prev_short_sma_ != 0.0 && prev_long_sma_ != 0.0) {
        bool was_below = prev_short_sma_ <= prev_long_sma_;
        bool now_above = cur_short > cur_long;
        if (was_below && now_above)  sig = Signal::BUY;
        else if (!was_below && !now_above) sig = Signal::SELL;
    }
    prev_short_sma_ = cur_short;
    prev_long_sma_  = cur_long;
    return sig;
}

std::string MACrossoverStrategy::name() const {
    return "MA Crossover(" + std::to_string(short_period_) + "/" +
           std::to_string(long_period_) + ")";
}

// ---------------------------------------------------------------------------
// RSI Strategy
// ---------------------------------------------------------------------------
RSIStrategy::RSIStrategy(size_t period, double oversold, double overbought)
    : period_(period), oversold_(oversold), overbought_(overbought) {}

Signal RSIStrategy::evaluate(const Stock& stock, const Portfolio& /*portfolio*/) {
    auto prices = stock.closingPrices();
    if (prices.size() < period_ + 2) return Signal::HOLD;

    double rsi_val = Indicators::rsi(prices, period_);
    if (rsi_val < oversold_)   return Signal::BUY;
    if (rsi_val > overbought_) return Signal::SELL;
    return Signal::HOLD;
}

std::string RSIStrategy::name() const {
    return "RSI(" + std::to_string(period_) + " | " +
           std::to_string(static_cast<int>(oversold_)) + "/" +
           std::to_string(static_cast<int>(overbought_)) + ")";
}

// ---------------------------------------------------------------------------
// Bollinger Bands Strategy
// ---------------------------------------------------------------------------
BollingerStrategy::BollingerStrategy(size_t period, double k)
    : period_(period), k_(k) {}

Signal BollingerStrategy::evaluate(const Stock& stock, const Portfolio& /*portfolio*/) {
    auto prices = stock.closingPrices();
    if (prices.size() < period_) return Signal::HOLD;

    auto bb = Indicators::bollingerBands(prices, period_, k_);
    double cur = stock.price();
    if (cur <= bb.lower) return Signal::BUY;
    if (cur >= bb.upper) return Signal::SELL;
    return Signal::HOLD;
}

std::string BollingerStrategy::name() const {
    std::ostringstream oss;
    oss << "Bollinger(" << period_ << ", k=" << std::fixed << std::setprecision(1) << k_ << ")";
    return oss.str();
}

// ---------------------------------------------------------------------------
// MACD Strategy
// ---------------------------------------------------------------------------
MACDStrategy::MACDStrategy(size_t fast, size_t slow, size_t signal)
    : fast_(fast), slow_(slow), signal_(signal), prev_histogram_(0.0) {}

Signal MACDStrategy::evaluate(const Stock& stock, const Portfolio& /*portfolio*/) {
    auto prices = stock.closingPrices();
    auto m = Indicators::macd(prices, fast_, slow_, signal_);

    Signal sig = Signal::HOLD;
    if (prev_histogram_ != 0.0) {
        if (prev_histogram_ <= 0.0 && m.histogram > 0.0) sig = Signal::BUY;
        else if (prev_histogram_ >= 0.0 && m.histogram < 0.0) sig = Signal::SELL;
    }
    prev_histogram_ = m.histogram;
    return sig;
}

std::string MACDStrategy::name() const {
    return "MACD(" + std::to_string(fast_) + "/" +
           std::to_string(slow_) + "/" + std::to_string(signal_) + ")";
}
