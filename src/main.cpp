#include "market_simulator.h"
#include "portfolio.h"
#include "strategy.h"
#include "indicators.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

// ---------------------------------------------------------------------------
// Formatting helpers
// ---------------------------------------------------------------------------
static std::string fmtMoney(double v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    if (v < 0) oss << "-$" << -v;
    else        oss << "$" << v;
    return oss.str();
}

static std::string fmtPct(double v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << v << "%";
    return oss.str();
}

static std::string signalStr(Signal s) {
    switch (s) {
        case Signal::BUY:  return "BUY ";
        case Signal::SELL: return "SELL";
        default:           return "HOLD";
    }
}

// ---------------------------------------------------------------------------
// Print a summary header
// ---------------------------------------------------------------------------
static void printHeader(const std::string& text) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << text << "\n";
    std::cout << std::string(60, '=') << "\n";
}

// ---------------------------------------------------------------------------
// Print portfolio snapshot
// ---------------------------------------------------------------------------
static void printPortfolio(const Portfolio& portfolio,
                            const std::map<std::string, double>& prices,
                            double initial_value) {
    double total    = portfolio.totalValue(prices);
    double unrealised = portfolio.unrealisedPnL(prices);
    double realised   = portfolio.realisedPnL();
    double total_ret  = (total / initial_value - 1.0) * 100.0;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Cash:              " << fmtMoney(portfolio.cash()) << "\n";
    std::cout << "  Positions value:   " << fmtMoney(portfolio.positionsValue(prices)) << "\n";
    std::cout << "  Total value:       " << fmtMoney(total) << "\n";
    std::cout << "  Unrealised P&L:    " << fmtMoney(unrealised) << "\n";
    std::cout << "  Realised P&L:      " << fmtMoney(realised) << "\n";
    std::cout << "  Total return:      " << fmtPct(total_ret) << "\n";

    if (!portfolio.positions().empty()) {
        std::cout << "\n  Open positions:\n";
        std::cout << "  " << std::left
                  << std::setw(8)  << "Symbol"
                  << std::setw(10) << "Shares"
                  << std::setw(14) << "Avg Cost"
                  << std::setw(14) << "Cur Price"
                  << std::setw(14) << "Mkt Value"
                  << "P&L\n";
        std::cout << "  " << std::string(56, '-') << "\n";
        for (const auto& [sym, pos] : portfolio.positions()) {
            auto it = prices.find(sym);
            double cur = (it != prices.end()) ? it->second : pos.avg_cost;
            double mkt = cur * pos.shares;
            double pnl = (cur - pos.avg_cost) * pos.shares;
            std::cout << "  " << std::left
                      << std::setw(8)  << sym
                      << std::setw(10) << pos.shares
                      << std::setw(14) << fmtMoney(pos.avg_cost)
                      << std::setw(14) << fmtMoney(cur)
                      << std::setw(14) << fmtMoney(mkt)
                      << fmtMoney(pnl) << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Print trade history summary
// ---------------------------------------------------------------------------
static void printTradeHistory(const Portfolio& portfolio) {
    const auto& trades = portfolio.tradeHistory();
    if (trades.empty()) {
        std::cout << "  No trades executed.\n";
        return;
    }
    std::cout << "  Total trades: " << trades.size() << "\n";
    int buys = 0, sells = 0;
    for (const auto& t : trades) {
        if (t.type == OrderType::BUY) ++buys; else ++sells;
    }
    std::cout << "  Buys:  " << buys  << "\n";
    std::cout << "  Sells: " << sells << "\n";

    std::cout << "\n  Last 10 trades:\n";
    std::cout << "  " << std::left
              << std::setw(8)  << "Symbol"
              << std::setw(6)  << "Type"
              << std::setw(10) << "Shares"
              << std::setw(14) << "Price"
              << "Value\n";
    std::cout << "  " << std::string(44, '-') << "\n";
    size_t start = trades.size() > 10 ? trades.size() - 10 : 0;
    for (size_t i = start; i < trades.size(); ++i) {
        const auto& t = trades[i];
        std::string ttype = (t.type == OrderType::BUY) ? "BUY" : "SELL";
        std::cout << "  " << std::left
                  << std::setw(8)  << t.symbol
                  << std::setw(6)  << ttype
                  << std::setw(10) << t.shares
                  << std::setw(14) << fmtMoney(t.price)
                  << fmtMoney(t.price * t.shares) << "\n";
    }
}

// ---------------------------------------------------------------------------
// Per-stock indicators snapshot
// ---------------------------------------------------------------------------
static void printIndicators(const Stock& stock) {
    auto prices = stock.closingPrices();
    if (prices.size() < 30) return;

    double sma10  = Indicators::sma(prices, 10);
    double sma30  = Indicators::sma(prices, 30);
    double rsi14  = Indicators::rsi(prices, 14);
    auto   bb     = Indicators::bollingerBands(prices, 20, 2.0);
    auto   m      = Indicators::macd(prices, 12, 26, 9);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "    Price:        " << fmtMoney(stock.price()) << "\n";
    std::cout << "    SMA(10):      " << fmtMoney(sma10) << "\n";
    std::cout << "    SMA(30):      " << fmtMoney(sma30) << "\n";
    std::cout << "    RSI(14):      " << std::setprecision(1) << rsi14 << "\n";
    std::cout << "    BB upper:     " << fmtMoney(bb.upper) << "\n";
    std::cout << "    BB lower:     " << fmtMoney(bb.lower) << "\n";
    std::cout << "    MACD line:    " << std::setprecision(4) << m.macd_line << "\n";
    std::cout << "    MACD hist:    " << std::setprecision(4) << m.histogram << "\n";
}

// ---------------------------------------------------------------------------
// Strategy runner: pairs a stock with a strategy
// ---------------------------------------------------------------------------
struct StrategyRunner {
    Stock*                   stock;
    std::unique_ptr<Strategy> strategy;
    std::string               name;
};

// ---------------------------------------------------------------------------
// Main simulation
// ---------------------------------------------------------------------------
int main() {
    printHeader("Stock Market Trading Bot - C++ Simulator");

    const double INITIAL_CASH       = 100000.0;  // $100,000 starting capital
    const double COMMISSION         = 4.95;       // $4.95 per trade
    const int    WARMUP_BARS        = 50;         // bars before trading starts
    const int    SIMULATION_BARS    = 300;        // total bars to simulate
    const int    REPORT_INTERVAL    = 50;         // print snapshot every N bars

    // ------------------------------------------------------------------
    // Set up market with several stocks
    // ------------------------------------------------------------------
    MarketSimulator market(/*seed=*/2024);
    market.addStock("AAPL", "Apple Inc.",          180.0, 0.12, 0.28);
    market.addStock("MSFT", "Microsoft Corp.",     380.0, 0.14, 0.22);
    market.addStock("NVDA", "NVIDIA Corp.",        700.0, 0.20, 0.45);
    market.addStock("AMZN", "Amazon.com Inc.",     185.0, 0.10, 0.30);
    market.addStock("TSLA", "Tesla Inc.",          250.0, 0.08, 0.55);

    std::cout << "\nStocks in simulation:\n";
    for (const auto& s : market.stocks()) {
        std::cout << "  " << std::left << std::setw(6) << s->symbol()
                  << " " << std::setw(26) << s->name()
                  << " Initial: " << fmtMoney(s->price()) << "\n";
    }

    // ------------------------------------------------------------------
    // Set up portfolio and strategies (one strategy per stock)
    // ------------------------------------------------------------------
    Portfolio portfolio(INITIAL_CASH, COMMISSION);
    const double initial_value = INITIAL_CASH;

    std::vector<StrategyRunner> runners;
    runners.push_back({market.getStock("AAPL"), std::make_unique<MACrossoverStrategy>(10, 30), "AAPL"});
    runners.push_back({market.getStock("MSFT"), std::make_unique<RSIStrategy>(14, 30.0, 70.0),  "MSFT"});
    runners.push_back({market.getStock("NVDA"), std::make_unique<BollingerStrategy>(20, 2.0),   "NVDA"});
    runners.push_back({market.getStock("AMZN"), std::make_unique<MACDStrategy>(12, 26, 9),       "AMZN"});
    runners.push_back({market.getStock("TSLA"), std::make_unique<MACrossoverStrategy>(5, 20),    "TSLA"});

    std::cout << "\nTrading strategies:\n";
    for (const auto& r : runners)
        std::cout << "  " << std::left << std::setw(6) << r.name
                  << " -> " << r.strategy->name() << "\n";

    std::cout << "\nStarting capital:   " << fmtMoney(INITIAL_CASH) << "\n";
    std::cout << "Commission/trade:   " << fmtMoney(COMMISSION) << "\n";
    std::cout << "Warmup bars:        " << WARMUP_BARS << "\n";
    std::cout << "Simulation bars:    " << SIMULATION_BARS << "\n";

    // ------------------------------------------------------------------
    // Simulation loop
    // ------------------------------------------------------------------
    std::cout << "\nRunning simulation...\n";
    int total_signals = 0;

    for (int bar = 1; bar <= SIMULATION_BARS; ++bar) {
        market.step();
        auto prices = market.currentPrices();

        // Only trade after warmup period
        if (bar > WARMUP_BARS) {
            for (auto& runner : runners) {
                if (!runner.stock) continue;
                Signal sig = runner.strategy->evaluate(*runner.stock, portfolio);

                if (sig == Signal::BUY) {
                    int shares = runner.strategy->orderSize(*runner.stock, portfolio);
                    if (shares > 0) {
                        bool ok = portfolio.buy(runner.stock->symbol(),
                                                runner.stock->price(), shares);
                        if (ok) {
                            ++total_signals;
                            if (bar % REPORT_INTERVAL == 0 || bar == SIMULATION_BARS)
                                std::cout << "  Bar " << std::setw(4) << bar
                                          << " [" << runner.stock->symbol() << "] "
                                          << signalStr(sig) << " " << shares
                                          << " @ " << fmtMoney(runner.stock->price()) << "\n";
                        }
                    }
                } else if (sig == Signal::SELL) {
                    int shares_held = portfolio.sharesHeld(runner.stock->symbol());
                    if (shares_held > 0) {
                        bool ok = portfolio.sell(runner.stock->symbol(),
                                                 runner.stock->price(), shares_held);
                        if (ok) {
                            ++total_signals;
                            if (bar % REPORT_INTERVAL == 0 || bar == SIMULATION_BARS)
                                std::cout << "  Bar " << std::setw(4) << bar
                                          << " [" << runner.stock->symbol() << "] "
                                          << signalStr(sig) << " " << shares_held
                                          << " @ " << fmtMoney(runner.stock->price()) << "\n";
                        }
                    }
                }
            }
        }

        // Periodic report
        if (bar % REPORT_INTERVAL == 0) {
            std::cout << "\n--- Bar " << bar << " portfolio snapshot ---\n";
            std::cout << "  Total value: " << fmtMoney(portfolio.totalValue(prices))
                      << "  (return: "
                      << fmtPct((portfolio.totalValue(prices) / initial_value - 1.0) * 100.0)
                      << ")\n";
        }
    }

    auto final_prices = market.currentPrices();

    // ------------------------------------------------------------------
    // Final report
    // ------------------------------------------------------------------
    printHeader("Final Portfolio Report");
    printPortfolio(portfolio, final_prices, initial_value);

    printHeader("Trade History");
    printTradeHistory(portfolio);

    printHeader("Final Market Indicators");
    for (const auto& s : market.stocks()) {
        std::cout << "\n  " << s->symbol() << " - " << s->name() << "\n";
        printIndicators(*s);
    }

    printHeader("Performance Summary");
    double final_total = portfolio.totalValue(final_prices);
    double total_return_pct = (final_total / initial_value - 1.0) * 100.0;
    std::cout << "  Initial capital:   " << fmtMoney(initial_value) << "\n";
    std::cout << "  Final value:       " << fmtMoney(final_total) << "\n";
    std::cout << "  Total return:      " << fmtPct(total_return_pct) << "\n";
    std::cout << "  Realised P&L:      " << fmtMoney(portfolio.realisedPnL()) << "\n";
    std::cout << "  Unrealised P&L:    " << fmtMoney(portfolio.unrealisedPnL(final_prices)) << "\n";
    std::cout << "  Total trades:      " << portfolio.tradeHistory().size() << "\n";
    std::cout << "  Bars simulated:    " << SIMULATION_BARS << "\n";
    std::cout << "\n";
    std::cout << (total_return_pct >= 0 ? "  [+] Strategy was profitable!\n"
                                        : "  [-] Strategy lost money.\n");
    std::cout << "\n";

    return 0;
}
