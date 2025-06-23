# C++ Modular Back-Testing Engine

## Table of Contents
1. Overview
2. High-Level Architecture
3. Module Breakdown & Task Quantification
4. Build & Run
5. Contributing
6. License

---

## 1. Overview
A high-performance, plug-and-play back-testing framework written in **C++20**. The engine is designed to:
* Fetch historical market data (OHLCV)
* Aggregate raw data into arbitrary bar resolutions
* Execute multiple trading strategies concurrently
* Record trades & compute detailed performance metrics
* Scale with available CPU cores via a lightweight thread pool

## 2. High-Level Architecture
```
PriceSource ─► Aggregator ─► ExecutionEngine ─► IStrategy (N instances, parallel)
       ▲                                    ▲                    │
       │                                    └──── OrderRequests ◄─┘
       └─────────────────────────────── Trades / AccountSnapshots ─► Metrics
```

Directory layout (planned):
```
include/
  core/      Bar.h, OrderRequest.h, Trade.h, Position.h, Metrics.h
  data/      PriceSource.h, Aggregator.h
  strategy/  IStrategy.h
  engine/    ExecutionEngine.h, ThreadPool.h, BacktestManager.h
src/          (mirrors include/ hierarchy)
main.cpp      CLI entry-point
```

## 3. Module Breakdown & Task Quantification
Below is a **task list** for each module. ✔️ marks completed tasks (to be updated as development progresses).

### 3.1 Core Data Structures (`include/core`, `src/core`)
| # | Task | Status |
|---|------|--------|
| 1 | Design `Bar` (timestamp, open, high, low, close, volume) | ⬜ |
| 2 | Design `OrderRequest` (side, sizeUsd, sizeAmount, leverage, SL, TP) | ⬜ |
| 3 | Design `Trade`, `Position`, `AccountSnapshot` | ⬜ |
| 4 | Implement serialization helpers (fmt / ostream) | ⬜ |
| 5 | Unit tests for value semantics & math operations | ⬜ |

### 3.2 Data Layer (`include/data`, `src/data`)
| # | Task | Status |
|---|------|--------|
| 1 | Define `PriceSource` interface | ⬜ |
| 2 | Implement CSV price source (baseline) | ⬜ |
| 3 | Implement HTTP-API price source (cpr) | ⬜ |
| 4 | Create streaming `Aggregator` (N-min bars) | ⬜ |
| 5 | Benchmarks & edge-case tests (leap seconds, DST) | ⬜ |

### 3.3 Strategy Layer (`include/strategy`, `src/strategy`)
| # | Task | Status |
|---|------|--------|
| 1 | Define `IStrategy` virtual interface | ⬜ |
| 2 | Provide example MA-Crossover strategy | ⬜ |
| 3 | Provide example Momentum strategy | ⬜ |
| 4 | Add unit tests with stub data | ⬜ |

### 3.4 Execution Engine (`include/engine`, `src/engine`)
| # | Task | Status |
|---|------|--------|
| 1 | Implement `ExecutionEngine` skeleton (bar loop) | ⬜ |
| 2 | Integrate thread pool for parallel strategy calls | ⬜ |
| 3 | Order validation & margin checks | ⬜ |
| 4 | Trade matching & account equity updates | ⬜ |
| 5 | Position life-cycle management (SL/TP) | ⬜ |
| 6 | Generate `AccountSnapshot` ledger | ⬜ |
| 7 | Fuzz tests with randomized orders | ⬜ |

### 3.5 Concurrency Infrastructure (`include/engine/ThreadPool.h`, `src/engine/ThreadPool.cpp`)
| # | Task | Status |
|---|------|--------|
| 1 | Design fixed-size thread pool (std::jthread & queue) | ⬜ |
| 2 | Support task futures & exception propagation | ⬜ |
| 3 | Micro-benchmarks (vs. naive serial execution) | ⬜ |

### 3.6 Metrics Layer (`include/core/Metrics.h`, `src/core/Metrics.cpp`)
| # | Task | Status |
|---|------|--------|
| 1 | Implement incremental PnL calculator | ⬜ |
| 2 | Sharpe ratio (annualized) | ⬜ |
| 3 | Sortino ratio | ⬜ |
| 4 | Max drawdown | ⬜ |
| 5 | Profit factor | ⬜ |
| 6 | Net & Gross P/L (long vs short) | ⬜ |
| 7 | JSON/CSV exporter for results | ⬜ |

### 3.7 Orchestration & CLI (`BacktestManager`, `main.cpp`)
| # | Task | Status |
|---|------|--------|
| 1 | Parse CLI flags (price path, timeframe, strategy list) | ⬜ |
| 2 | Load YAML/JSON config for strategies & params | ⬜ |
| 3 | Initialize modules and kick off back-test | ⬜ |
| 4 | Aggregate & pretty-print summary table | ⬜ |
| 5 | Optionally write detailed logs to disk | ⬜ |

### 3.8 Documentation & Examples
| # | Task | Status |
|---|------|--------|
| 1 | Update this README as tasks progress | ⬜ |
| 2 | Provide Doxygen comments & generate docs | ⬜ |
| 3 | Create `examples/ema_crossover.cpp` showcase | ⬜ |
| 4 | Add GIF / screenshot of sample output | ⬜ |


## 4. Build & Run
```bash
# Clone & build (requires CMake ≥3.20, g++-12 or clang++-15)
git clone https://github.com/yourname/backtest-engine.git
cd backtest-engine
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run a demo back-test (using sample CSV)
./backtest_engine \
  --data ../data/BTCUSDT.csv \
  --resolution 5 \
  --strategy ema_crossover.json
```

## 5. Contributing
Pull requests are welcome! Please open an issue to discuss big changes. Make sure your code passes `clang-tidy` and `ctest`.

## 6. License
MIT 