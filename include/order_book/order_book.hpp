#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <array>
#include <limits>

namespace trading {

// Price levels are represented in fixed-point notation
using Price = int64_t;
using Quantity = int64_t;
using OrderId = uint64_t;

struct Order {
    OrderId id;
    Price price;
    Quantity quantity;
    bool is_buy;
    std::atomic<Order*> next;

    Order(OrderId id_, Price price_, Quantity quantity_, bool is_buy_)
        : id(id_), price(price_), quantity(quantity_), is_buy(is_buy_), next(nullptr) {}
};

class LockFreeOrderBook {
public:
    static constexpr size_t MAX_PRICE_LEVELS = 10000;  // Configurable based on needs

    LockFreeOrderBook();
    ~LockFreeOrderBook();

    // Non-copyable
    LockFreeOrderBook(const LockFreeOrderBook&) = delete;
    LockFreeOrderBook& operator=(const LockFreeOrderBook&) = delete;

    // Core functionality
    bool addOrder(OrderId id, Price price, Quantity quantity, bool is_buy) noexcept;
    bool cancelOrder(OrderId id) noexcept;
    bool modifyOrder(OrderId id, Quantity new_quantity) noexcept;

    // Market data queries
    Price getBestBid() const noexcept;
    Price getBestAsk() const noexcept;
    Quantity getQuantityAtPrice(Price price, bool is_buy) const noexcept;

private:
    struct PriceLevel {
        std::atomic<Order*> head;
        std::atomic<Quantity> total_quantity;

        PriceLevel() : head(nullptr), total_quantity(0) {}
    };

    std::array<PriceLevel, MAX_PRICE_LEVELS> buy_levels_;
    std::array<PriceLevel, MAX_PRICE_LEVELS> sell_levels_;
    
    std::atomic<Price> best_bid_;
    std::atomic<Price> best_ask_;

    // Helper methods
    size_t getPriceLevelIndex(Price price) const noexcept;
    void updateBestPrices() noexcept;
};

} // namespace trading
