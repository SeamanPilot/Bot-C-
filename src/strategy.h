#pragma once
#include "stock.h"
#include "portfolio.h"
#include <string>
#include <memory>

// Signal returned by a strategy on each bar
enum class Signal { BUY, SELL, HOLD };

// Abstract base class for all trading strategies
class Strategy {
public:
    virtual ~Strategy() = default;

    // Generate a signal given the current stock and portfolio state
    virtual Signal evaluate(const Stock& stock, const Portfolio& portfolio) = 0;

    virtual std::string name() const = 0;

    // How many shares to trade when a signal fires
    virtual int orderSize(const Stock& stock, const Portfolio& portfolio) const;

protected:
    // By default allocate up to `fraction` of available cash per position
    static constexpr double DEFAULT_POSITION_FRACTION = 0.25;
};

// Moving Average Crossover strategy:
//   BUY  when short SMA crosses above long SMA
//   SELL when short SMA crosses below long SMA
class MACrossoverStrategy : public Strategy {
public:
    MACrossoverStrategy(size_t short_period = 10, size_t long_period = 30);
    Signal evaluate(const Stock& stock, const Portfolio& portfolio) override;
    std::string name() const override;

private:
    size_t short_period_;
    size_t long_period_;
    double prev_short_sma_;
    double prev_long_sma_;
};

// RSI strategy:
//   BUY  when RSI < oversold threshold
//   SELL when RSI > overbought threshold
class RSIStrategy : public Strategy {
public:
    RSIStrategy(size_t period = 14, double oversold = 30.0, double overbought = 70.0);
    Signal evaluate(const Stock& stock, const Portfolio& portfolio) override;
    std::string name() const override;

private:
    size_t period_;
    double oversold_;
    double overbought_;
};

// Bollinger Bands strategy:
//   BUY  when price touches lower band
//   SELL when price touches upper band
class BollingerStrategy : public Strategy {
public:
    BollingerStrategy(size_t period = 20, double k = 2.0);
    Signal evaluate(const Stock& stock, const Portfolio& portfolio) override;
    std::string name() const override;

private:
    size_t period_;
    double k_;
};

// MACD strategy:
//   BUY  when MACD histogram crosses above 0
//   SELL when MACD histogram crosses below 0
class MACDStrategy : public Strategy {
public:
    MACDStrategy(size_t fast = 12, size_t slow = 26, size_t signal = 9);
    Signal evaluate(const Stock& stock, const Portfolio& portfolio) override;
    std::string name() const override;

private:
    size_t fast_;
    size_t slow_;
    size_t signal_;
    double prev_histogram_;
};
