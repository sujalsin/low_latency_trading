# Low-Latency Trading System Prototype

A high-performance trading system prototype implemented in C++, featuring lock-free data structures and custom network protocols for minimal latency.

## Features

- Lock-free order book implementation using C++11 atomics
- Custom UDP socket implementation for market data dissemination
- Zero-copy message handling
- Memory-efficient price level management
- Thread-safe order modifications and cancellations

## Components

1. **Order Book (`include/order_book/order_book.hpp`)**
   - Lock-free implementation using atomic operations
   - O(1) order insertion and cancellation
   - Efficient price level management
   - Thread-safe best bid/ask tracking

2. **Network Layer (`include/network/udp_socket.hpp`)**
   - Custom UDP socket implementation
   - Non-blocking I/O
   - Configurable buffer sizes
   - Error handling with std::error_code

## Building the Project

```bash
mkdir build
cd build
cmake ..
make
```

## Requirements

- C++17 compatible compiler
- CMake 3.10 or higher
- POSIX-compliant operating system

## Usage Example

The main.cpp file demonstrates basic usage of the system:

```cpp
// Create order book
LockFreeOrderBook order_book;

// Add orders
order_book.addOrder(1, 100, 10, true);   // Buy 10 @ 100
order_book.addOrder(2, 101, 20, true);   // Buy 20 @ 101

// Modify orders
order_book.modifyOrder(2, 30);  // Modify quantity to 30

// Cancel orders
order_book.cancelOrder(1);  // Cancel order 1
```

## Performance Considerations

1. **Lock-Free Design**
   - Minimizes thread contention
   - Eliminates mutex overhead
   - Provides wait-free progress guarantee for critical operations

2. **Memory Management**
   - Pre-allocated price levels
   - Zero-copy message handling
   - Cache-friendly data structures

3. **Network Optimization**
   - Non-blocking I/O
   - Configurable buffer sizes
   - Minimal system calls

## Future Improvements

1. Implement a more sophisticated price level mapping
2. Add TCP support for order entry
3. Implement market data compression
4. Add performance metrics and monitoring
5. Implement matching engine
6. Add risk management system

## License

MIT License
