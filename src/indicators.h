#pragma once
#include <vector>
#include <stdexcept>
#include <cmath>

// Technical analysis indicators

namespace Indicators {

// Simple Moving Average
inline double sma(const std::vector<double>& prices, size_t period) {
    if (prices.size() < period || period == 0)
        return 0.0;
    double sum = 0.0;
    for (size_t i = prices.size() - period; i < prices.size(); ++i)
        sum += prices[i];
    return sum / static_cast<double>(period);
}

// Exponential Moving Average for the full series; returns series of EMAs
inline std::vector<double> ema(const std::vector<double>& prices, size_t period) {
    if (prices.size() < period || period == 0)
        return {};
    std::vector<double> result;
    result.reserve(prices.size() - period + 1);
    // Seed with SMA of first `period` values
    double seed = 0.0;
    for (size_t i = 0; i < period; ++i) seed += prices[i];
    seed /= static_cast<double>(period);
    result.push_back(seed);
    double k = 2.0 / (static_cast<double>(period) + 1.0);
    for (size_t i = period; i < prices.size(); ++i) {
        seed = prices[i] * k + seed * (1.0 - k);
        result.push_back(seed);
    }
    return result;
}

// Latest EMA value
inline double emaLatest(const std::vector<double>& prices, size_t period) {
    auto series = ema(prices, period);
    return series.empty() ? 0.0 : series.back();
}

// Relative Strength Index (RSI)
inline double rsi(const std::vector<double>& prices, size_t period = 14) {
    if (prices.size() < period + 1 || period == 0)
        return 50.0; // neutral if insufficient data
    double gains = 0.0, losses = 0.0;
    // Initial average gain/loss over first `period` changes
    for (size_t i = prices.size() - period; i < prices.size(); ++i) {
        double change = prices[i] - prices[i - 1];
        if (change > 0) gains  += change;
        else            losses -= change;
    }
    double avg_gain = gains  / static_cast<double>(period);
    double avg_loss = losses / static_cast<double>(period);
    if (avg_loss == 0.0) return 100.0;
    double rs = avg_gain / avg_loss;
    return 100.0 - (100.0 / (1.0 + rs));
}

struct BollingerBands {
    double upper;
    double middle; // SMA
    double lower;
};

// Bollinger Bands: middle = SMA(n), upper/lower = middle ± k*stddev
inline BollingerBands bollingerBands(const std::vector<double>& prices,
                                     size_t period = 20, double k = 2.0) {
    if (prices.size() < period || period == 0)
        return {0.0, 0.0, 0.0};
    double mid = sma(prices, period);
    double variance = 0.0;
    for (size_t i = prices.size() - period; i < prices.size(); ++i) {
        double diff = prices[i] - mid;
        variance += diff * diff;
    }
    double stddev = std::sqrt(variance / static_cast<double>(period));
    return {mid + k * stddev, mid, mid - k * stddev};
}

struct MACD {
    double macd_line;   // fast EMA - slow EMA
    double signal_line; // EMA of macd_line
    double histogram;   // macd_line - signal_line
};

// MACD indicator
inline MACD macd(const std::vector<double>& prices,
                 size_t fast_period = 12, size_t slow_period = 26,
                 size_t signal_period = 9) {
    if (prices.size() < slow_period + signal_period)
        return {0.0, 0.0, 0.0};
    auto fast_ema = ema(prices, fast_period);
    auto slow_ema = ema(prices, slow_period);
    // Align series: slow_ema is shorter; align from the right
    size_t slow_size = slow_ema.size();
    size_t fast_size = fast_ema.size();
    if (slow_size > fast_size) return {0.0, 0.0, 0.0};
    std::vector<double> macd_series;
    macd_series.reserve(slow_size);
    size_t fast_offset = fast_size - slow_size;
    for (size_t i = 0; i < slow_size; ++i)
        macd_series.push_back(fast_ema[fast_offset + i] - slow_ema[i]);
    if (macd_series.size() < signal_period)
        return {0.0, 0.0, 0.0};
    auto signal_series = ema(macd_series, signal_period);
    double macd_val   = macd_series.back();
    double signal_val = signal_series.empty() ? 0.0 : signal_series.back();
    return {macd_val, signal_val, macd_val - signal_val};
}

} // namespace Indicators
