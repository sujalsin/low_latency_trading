#include "order_book/order_book.hpp"
#include <algorithm>
#include <limits>

namespace trading {

LockFreeOrderBook::LockFreeOrderBook()
    : best_bid_(std::numeric_limits<Price>::min())
    , best_ask_(std::numeric_limits<Price>::max()) {
}

LockFreeOrderBook::~LockFreeOrderBook() {
    // Cleanup all orders in both buy and sell levels
    for (auto& level : buy_levels_) {
        Order* current = level.head.load(std::memory_order_acquire);
        while (current) {
            Order* next = current->next.load(std::memory_order_acquire);
            delete current;
            current = next;
        }
    }

    for (auto& level : sell_levels_) {
        Order* current = level.head.load(std::memory_order_acquire);
        while (current) {
            Order* next = current->next.load(std::memory_order_acquire);
            delete current;
            current = next;
        }
    }
}

bool LockFreeOrderBook::addOrder(OrderId id, Price price, Quantity quantity, bool is_buy) noexcept {
    if (quantity <= 0) return false;

    size_t level_idx = getPriceLevelIndex(price);
    auto& levels = is_buy ? buy_levels_ : sell_levels_;
    
    // Create new order
    Order* new_order = new Order(id, price, quantity, is_buy);
    
    // Add to price level using CAS loop
    while (true) {
        Order* old_head = levels[level_idx].head.load(std::memory_order_acquire);
        new_order->next.store(old_head, std::memory_order_release);
        
        if (levels[level_idx].head.compare_exchange_weak(old_head, new_order,
                                                       std::memory_order_acq_rel)) {
            break;
        }
    }

    // Update total quantity atomically
    levels[level_idx].total_quantity.fetch_add(quantity, std::memory_order_acq_rel);
    
    // Update best prices
    if (is_buy) {
        Price current_best = best_bid_.load(std::memory_order_acquire);
        while (price > current_best) {
            if (best_bid_.compare_exchange_weak(current_best, price,
                                              std::memory_order_acq_rel)) {
                break;
            }
        }
    } else {
        Price current_best = best_ask_.load(std::memory_order_acquire);
        while (price < current_best) {
            if (best_ask_.compare_exchange_weak(current_best, price,
                                              std::memory_order_acq_rel)) {
                break;
            }
        }
    }

    return true;
}

bool LockFreeOrderBook::cancelOrder(OrderId id) noexcept {
    // Search both buy and sell levels
    for (auto& levels : {std::ref(buy_levels_), std::ref(sell_levels_)}) {
        for (size_t i = 0; i < MAX_PRICE_LEVELS; ++i) {
            Order* prev = nullptr;
            Order* current = levels.get()[i].head.load(std::memory_order_acquire);
            
            while (current) {
                if (current->id == id) {
                    // Found the order to cancel
                    if (prev) {
                        // Update next pointer of previous order
                        Order* next = current->next.load(std::memory_order_acquire);
                        prev->next.store(next, std::memory_order_release);
                    } else {
                        // Update head of price level
                        Order* next = current->next.load(std::memory_order_acquire);
                        levels.get()[i].head.store(next, std::memory_order_release);
                    }
                    
                    // Update total quantity
                    levels.get()[i].total_quantity.fetch_sub(current->quantity,
                                                           std::memory_order_acq_rel);
                    
                    // Cleanup and update best prices
                    delete current;
                    updateBestPrices();
                    return true;
                }
                
                prev = current;
                current = current->next.load(std::memory_order_acquire);
            }
        }
    }
    
    return false;
}

bool LockFreeOrderBook::modifyOrder(OrderId id, Quantity new_quantity) noexcept {
    if (new_quantity <= 0) return false;

    // Search both buy and sell levels
    for (auto& levels : {std::ref(buy_levels_), std::ref(sell_levels_)}) {
        for (size_t i = 0; i < MAX_PRICE_LEVELS; ++i) {
            Order* current = levels.get()[i].head.load(std::memory_order_acquire);
            
            while (current) {
                if (current->id == id) {
                    // Found the order to modify
                    Quantity old_quantity = current->quantity;
                    current->quantity = new_quantity;
                    
                    // Update total quantity
                    levels.get()[i].total_quantity.fetch_add(new_quantity - old_quantity,
                                                           std::memory_order_acq_rel);
                    return true;
                }
                
                current = current->next.load(std::memory_order_acquire);
            }
        }
    }
    
    return false;
}

Price LockFreeOrderBook::getBestBid() const noexcept {
    return best_bid_.load(std::memory_order_acquire);
}

Price LockFreeOrderBook::getBestAsk() const noexcept {
    return best_ask_.load(std::memory_order_acquire);
}

Quantity LockFreeOrderBook::getQuantityAtPrice(Price price, bool is_buy) const noexcept {
    size_t level_idx = getPriceLevelIndex(price);
    const auto& levels = is_buy ? buy_levels_ : sell_levels_;
    return levels[level_idx].total_quantity.load(std::memory_order_acquire);
}

size_t LockFreeOrderBook::getPriceLevelIndex(Price price) const noexcept {
    // Simple modulo-based mapping for demo purposes
    // In production, use a more sophisticated mapping based on tick size and price range
    return static_cast<size_t>(price % MAX_PRICE_LEVELS);
}

void LockFreeOrderBook::updateBestPrices() noexcept {
    // Update best bid
    Price best_bid = std::numeric_limits<Price>::min();
    for (size_t i = 0; i < MAX_PRICE_LEVELS; ++i) {
        if (buy_levels_[i].total_quantity.load(std::memory_order_acquire) > 0) {
            Order* order = buy_levels_[i].head.load(std::memory_order_acquire);
            if (order && order->price > best_bid) {
                best_bid = order->price;
            }
        }
    }
    best_bid_.store(best_bid, std::memory_order_release);

    // Update best ask
    Price best_ask = std::numeric_limits<Price>::max();
    for (size_t i = 0; i < MAX_PRICE_LEVELS; ++i) {
        if (sell_levels_[i].total_quantity.load(std::memory_order_acquire) > 0) {
            Order* order = sell_levels_[i].head.load(std::memory_order_acquire);
            if (order && order->price < best_ask) {
                best_ask = order->price;
            }
        }
    }
    best_ask_.store(best_ask, std::memory_order_release);
}

} // namespace trading
