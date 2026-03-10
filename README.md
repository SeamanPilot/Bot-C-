# Bot-C-
A C++ stock market trading bot / simulator.

## Features

- **Market Simulation** – Realistic price generation using **Geometric Brownian Motion (GBM)** for multiple stocks simultaneously.
- **Technical Indicators** – Fully implemented in `src/indicators.h`:
  - Simple Moving Average (SMA)
  - Exponential Moving Average (EMA)
  - Relative Strength Index (RSI)
  - Bollinger Bands
  - MACD (Moving Average Convergence Divergence)
- **Trading Strategies** – Each with configurable parameters:
  - **MA Crossover** – Buys when the short SMA crosses above the long SMA; sells on the reverse.
  - **RSI** – Buys when RSI < oversold threshold; sells when RSI > overbought threshold.
  - **Bollinger Bands** – Buys when price touches the lower band; sells at the upper band.
  - **MACD** – Buys/sells on histogram zero-line crossovers.
- **Portfolio Management** – Tracks cash, open positions, average cost, unrealised/realised P&L, and full trade history.
- **Commission Model** – Configurable per-trade commission.
- **Performance Reporting** – Final portfolio snapshot, indicator values, trade history, and return summary.

## File Structure

```
src/
  indicators.h          – Technical analysis indicators (header-only)
  stock.h / stock.cpp   – Stock and OHLCV bar data structures
  portfolio.h / portfolio.cpp  – Portfolio manager
  strategy.h / strategy.cpp    – Trading strategies
  market_simulator.h / market_simulator.cpp  – GBM price simulator
  main.cpp              – Entry point and simulation loop
Makefile
```

## Build

Requires a C++17-capable compiler (GCC 8+ or Clang 7+).

```bash
make
```

## Run

```bash
./trading_bot
```

Or in one step:

```bash
make run
```

## Clean

```bash
make clean
```

## Example Output

```
============================================================
  Stock Market Trading Bot - C++ Simulator
============================================================

Stocks in simulation:
  AAPL   Apple Inc.                 Initial: $180.00
  MSFT   Microsoft Corp.            Initial: $380.00
  NVDA   NVIDIA Corp.               Initial: $700.00
  AMZN   Amazon.com Inc.            Initial: $185.00
  TSLA   Tesla Inc.                 Initial: $250.00

Trading strategies:
  AAPL   -> MA Crossover(10/30)
  MSFT   -> RSI(14 | 30/70)
  NVDA   -> Bollinger(20, k=2)
  AMZN   -> MACD(12/26/9)
  TSLA   -> MA Crossover(5/20)

  ...

============================================================
  Performance Summary
============================================================
  Initial capital:   $100000.00
  Final value:       $105125.14
  Total return:      5.13%
  Total trades:      108
  [+] Strategy was profitable!
```

## Customisation

Edit the constants at the top of `src/main.cpp`:

| Constant | Default | Description |
|---|---|---|
| `INITIAL_CASH` | 100000 | Starting capital ($) |
| `COMMISSION` | 4.95 | Per-trade commission ($) |
| `WARMUP_BARS` | 50 | Bars before trading begins |
| `SIMULATION_BARS` | 300 | Total bars to simulate |

Stocks can be added/removed via `market.addStock(symbol, name, price, drift, volatility)`.
Strategies are swappable — any `Strategy` subclass can be assigned to any stock.

