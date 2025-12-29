# Stock-Market-Project
A C++ stock market simulation application with real-time API integration and demo trading capabilities.

## Project Description
This C++ application connects to financial APIs to fetch real-time stock prices and provides a comprehensive demo trading platform. Users can simulate buying and selling stocks using virtual currency. The system includes three integrated databases for managing user accounts, stock data, and transaction history.

## Features
- Real-time API Integration: Connects to financial APIs for live stock prices
- Demo Trading System: Buy and sell stocks with virtual currency
- Multi-Database Architecture: Three databases for comprehensive data management
- Portfolio Tracking: Monitor investments and performance metrics
- Transaction History: Complete record of all trading activities

## Technical Stack
- Language: C++
- Libraries nlohmann,sqllite3
- Databases: Three databases stocks,User,Wallet systems

## How to Run?
### 1. Create build directory
```
mkdir build
cd build
```

### 2. Configure cmake
```
cmake ..
```

### 3. Build
```
make
```

### 4. Run!!
```
./main
```