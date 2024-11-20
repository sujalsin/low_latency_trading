#include "order_book/order_book.hpp"
#include "network/udp_socket.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace trading;
using namespace std::chrono_literals;

void printOrderBookStatus(const LockFreeOrderBook& book) {
    std::cout << "Best Bid: " << book.getBestBid() << "\n";
    std::cout << "Best Ask: " << book.getBestAsk() << "\n";
    std::cout << "Quantity at Best Bid: " 
              << book.getQuantityAtPrice(book.getBestBid(), true) << "\n";
    std::cout << "Quantity at Best Ask: " 
              << book.getQuantityAtPrice(book.getBestAsk(), false) << "\n";
    std::cout << "-------------------\n";
}

int main() {
    try {
        // Initialize order book
        LockFreeOrderBook order_book;

        // Create market data socket
        network::UDPSocket market_data_socket("127.0.0.1", 8001);
        market_data_socket.setNonBlocking(true);
        market_data_socket.setReceiveBufferSize(1024 * 1024);  // 1MB buffer

        // Add some test orders
        order_book.addOrder(1, 100, 10, true);   // Buy 10 @ 100
        order_book.addOrder(2, 101, 20, true);   // Buy 20 @ 101
        order_book.addOrder(3, 102, 15, false);  // Sell 15 @ 102
        order_book.addOrder(4, 103, 25, false);  // Sell 25 @ 103

        std::cout << "Initial Order Book State:\n";
        printOrderBookStatus(order_book);

        // Modify an order
        order_book.modifyOrder(2, 30);  // Modify order 2 to quantity 30
        std::cout << "After modifying order 2:\n";
        printOrderBookStatus(order_book);

        // Cancel an order
        order_book.cancelOrder(1);  // Cancel order 1
        std::cout << "After cancelling order 1:\n";
        printOrderBookStatus(order_book);

        // Demonstrate market data dissemination
        std::string market_data = "MARKET_UPDATE|BID=101,QTY=30|ASK=102,QTY=15";
        std::error_code ec;
        market_data_socket.sendData(market_data.c_str(), market_data.size(), ec);
        
        if (ec) {
            std::cerr << "Failed to send market data: " << ec.message() << "\n";
        } else {
            std::cout << "Market data sent successfully\n";
        }

        // Keep the program running for a bit
        std::this_thread::sleep_for(1s);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
