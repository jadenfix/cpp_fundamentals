#include <iostream>
#include <vector>
#include <string>
#include <chrono>    // For timestamps
#include <random>    // For generating random orders
#include <iomanip>   // For formatting output
#include <thread>    // For potential delays
#include <map>       // For storing price levels
#include <list>      // For storing orders at a price level (FIFO)
#include <limits>    // For numeric limits (e.g., infinity)
#include <functional>// For std::greater/std::less
#include <vector>    // Can be used instead of list if preferred
#include <cmath>     // For std::round
#include <stdexcept> // For exceptions
#include <algorithm> // For std::min

// --- Enums for Order Properties ---
enum class OrderSide { BUY, SELL };
enum class OrderType { LIMIT, MARKET };

// --- Forward Declarations ---
class OrderBook; // Needed for MatchingEngine constructor

// --- Order Structure (remains the same) ---
struct Order {
    long long orderId;
    OrderSide side;
    OrderType type;
    double price;
    int quantity;
    long long timestamp;
    int initialQuantity;

    Order(long long id, OrderSide s, OrderType t, double p, int q)
        : orderId(id), side(s), type(t), price(p), quantity(q), initialQuantity(q) {
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    }
    Order() : orderId(0), side(OrderSide::BUY), type(OrderType::LIMIT), price(0.0), quantity(0), timestamp(0), initialQuantity(0) {}

    void print(bool showTimestamp = true) const {
        std::cout << "ID:" << orderId
                  << " " << (side == OrderSide::BUY ? "BUY" : "SELL")
                  << " " << (type == OrderType::LIMIT ? "LIM" : "MKT")
                  << " Qty:" << quantity << "/" << initialQuantity;
        if (type == OrderType::LIMIT) {
            std::cout << " Px:" << std::fixed << std::setprecision(2) << price;
        }
        if (showTimestamp) {
             std::cout << " T:" << timestamp;
        }
    }
};

// --- Trade Structure (remains the same) ---
struct Trade {
    long long tradeId;
    long long aggressingOrderId;
    long long restingOrderId;
    double price;
    int quantity;
    long long timestamp;
    static long long nextTradeId;

    Trade(long long aggressingId, long long restingId, double p, int q)
        : aggressingOrderId(aggressingId), restingOrderId(restingId), price(p), quantity(q) {
        tradeId = nextTradeId++;
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    }

     void print() const {
        std::cout << "TRADE ID:" << tradeId
                  << " Aggressor:" << aggressingOrderId
                  << " Resting:" << restingOrderId
                  << " Qty:" << quantity
                  << " Price:" << std::fixed << std::setprecision(2) << price
                  << " Timestamp:" << timestamp; // Removed endl here
    }
};
long long Trade::nextTradeId = 1;


// --- Global Type Alias ---
// Moved OrderList definition outside the classes so it's visible to both
using OrderList = std::list<Order>;


// --- Order Book Class ---
class OrderBook {
private:
    // Bids map: Stores buy orders. Key = Price, Value = List of orders at that price.
    std::map<double, OrderList, std::greater<double>> bids;

    // Asks map: Stores sell orders. Key = Price, Value = List of orders at that price.
    std::map<double, OrderList, std::less<double>> asks;

public:
    void addOrder(const Order& order) {
        if (order.type != OrderType::LIMIT || order.quantity <= 0 || !std::isfinite(order.price) || order.price <= 0) {
            return; // Ignore invalid/market orders for direct book addition
        }
        if (order.side == OrderSide::BUY) {
            bids[order.price].push_back(order);
        } else {
            asks[order.price].push_back(order);
        }
    }

    // Basic removal by ID (inefficient, not used by matching engine)
    bool removeOrder(long long orderId) {
        for (auto& pair : bids) {
            auto& orderList = pair.second;
            for (auto it = orderList.begin(); it != orderList.end(); ++it) {
                if (it->orderId == orderId) {
                     std::cout << "Removing Order ID " << orderId << " from Bids at price " << std::fixed << std::setprecision(2) << it->price << std::endl;
                    orderList.erase(it);
                    if (orderList.empty()) bids.erase(pair.first);
                    return true;
                }
            }
        }
        for (auto& pair : asks) {
            auto& orderList = pair.second;
            for (auto it = orderList.begin(); it != orderList.end(); ++it) {
                if (it->orderId == orderId) {
                     std::cout << "Removing Order ID " << orderId << " from Asks at price " << std::fixed << std::setprecision(2) << it->price << std::endl;
                    orderList.erase(it);
                    if (orderList.empty()) asks.erase(pair.first);
                    return true;
                }
            }
        }
         std::cerr << "Warning: Could not find Order ID " << orderId << " to remove." << std::endl;
        return false;
    }

    double getBestBid() const {
        return bids.empty() ? 0.0 : bids.begin()->first;
    }

    double getBestAsk() const {
        return asks.empty() ? std::numeric_limits<double>::infinity() : asks.begin()->first;
    }

    bool bidsEmpty() const { return bids.empty(); }
    bool asksEmpty() const { return asks.empty(); }

    // Return reference to the list of orders at the best price
    // WARNING: Check !empty() before calling
    OrderList& getBestBidOrders() {
        if (bids.empty()) throw std::runtime_error("Bids empty in getBestBidOrders");
        return bids.begin()->second;
    }
    OrderList& getBestAskOrders() {
        if (asks.empty()) throw std::runtime_error("Asks empty in getBestAskOrders");
        return asks.begin()->second;
    }

    // Removes the price level if its order list becomes empty
    void cleanEmptyPriceLevel(OrderSide side, double price) {
        if (side == OrderSide::BUY) {
            auto it = bids.find(price);
            if (it != bids.end() && it->second.empty()) {
                bids.erase(it);
            }
        } else { // SELL
            auto it = asks.find(price);
            if (it != asks.end() && it->second.empty()) {
                asks.erase(it);
            }
        }
    }

    // Print book state (remains the same conceptually)
    void printBook() const {
         std::cout << "\n--- Order Book State ---" << std::endl;
         std::cout << "ASKS (Price | Total Qty):" << std::endl;
         if (asks.empty()) { std::cout << "  <Empty>" << std::endl; }
         else {
             std::vector<double> askPrices;
             for(const auto& pair : asks) askPrices.push_back(pair.first);
             // Print highest asks first visually (iterate reverse price order)
             for (auto it = askPrices.rbegin(); it != askPrices.rend(); ++it) {
                 double price = *it;
                 const auto& orderList = asks.at(price); // Use .at() for const correctness
                 int totalQuantity = 0;
                 for(const auto& order : orderList) totalQuantity += order.quantity;
                 std::cout << "  " << std::fixed << std::setprecision(2) << price << " | " << totalQuantity << " (";
                 for (const auto& order : orderList) { std::cout << order.orderId << ":" << order.quantity << " "; }
                 std::cout << ")" << std::endl;
             }
         }
         std::cout << "------------------------" << std::endl;
         std::cout << "BIDS (Price | Total Qty):" << std::endl;
         if (bids.empty()) { std::cout << "  <Empty>" << std::endl; }
         else {
             // Bids map is already sorted high-to-low price
             for (const auto& pair : bids) {
                 double price = pair.first;
                 const auto& orderList = pair.second;
                 int totalQuantity = 0;
                 for(const auto& order : orderList) totalQuantity += order.quantity;
                 std::cout << "  " << std::fixed << std::setprecision(2) << price << " | " << totalQuantity << " (";
                 for (const auto& order : orderList) { std::cout << order.orderId << ":" << order.quantity << " "; }
                 std::cout << ")" << std::endl;
             }
         }
         std::cout << "--- End Order Book ---" << std::endl;
    }
};


// --- Matching Engine Class (uses global OrderList now) ---
class MatchingEngine {
private:
    OrderBook& book;
    std::vector<Trade>& trades;

public:
    MatchingEngine(OrderBook& ob, std::vector<Trade>& tradeList)
        : book(ob), trades(tradeList) {}

    void processOrder(Order& incomingOrder) {
        std::cout << "Engine Processing: ";
        incomingOrder.print(false);
        std::cout << std::endl;

        if (incomingOrder.quantity <= 0) {
            std::cout << "  Ignoring order with zero or negative quantity." << std::endl;
            return;
        }

        if (incomingOrder.type == OrderType::LIMIT &&
            (!std::isfinite(incomingOrder.price) || incomingOrder.price <= 0)) {
            std::cout << "  Ignoring limit order with an invalid price." << std::endl;
            return;
        }

        if (incomingOrder.type == OrderType::LIMIT) {
            processLimitOrder(incomingOrder);
        } else { // OrderType::MARKET
            processMarketOrder(incomingOrder);
        }
    }

private:
    void processLimitOrder(Order& incomingOrder) {
        if (incomingOrder.side == OrderSide::BUY) {
            matchBuyOrder(incomingOrder);
        } else {
            matchSellOrder(incomingOrder);
        }

        if (incomingOrder.quantity > 0) {
            std::cout << "  Adding remaining Qty " << incomingOrder.quantity << " of LIMIT order " << incomingOrder.orderId << " to book." << std::endl;
            book.addOrder(incomingOrder);
        } else {
             std::cout << "  LIMIT order " << incomingOrder.orderId << " fully filled." << std::endl;
        }
    }

    void processMarketOrder(Order& incomingOrder) {
        if (incomingOrder.side == OrderSide::BUY) {
            matchBuyOrder(incomingOrder);
        } else {
            matchSellOrder(incomingOrder);
        }

        if (incomingOrder.quantity > 0) {
            std::cout << "  Market order " << incomingOrder.orderId << " partially filled. Remaining Qty "
                      << incomingOrder.quantity << " cancelled (insufficient liquidity)." << std::endl;
        } else {
             std::cout << "  Market order " << incomingOrder.orderId << " fully filled." << std::endl;
        }
    }

    // Uses global OrderList type now
    bool matchBuyOrder(Order& incomingBuyOrder) {
        bool matched = false;
        while (incomingBuyOrder.quantity > 0 && !book.asksEmpty() &&
               (incomingBuyOrder.type == OrderType::MARKET || incomingBuyOrder.price >= book.getBestAsk()))
        {
            double bestAskPrice = book.getBestAsk();
            OrderList& bestAskOrders = book.getBestAskOrders(); // This now works

            auto it = bestAskOrders.begin();
            while (it != bestAskOrders.end() && incomingBuyOrder.quantity > 0) {
                Order& restingSellOrder = *it;
                int tradeQuantity = std::min(incomingBuyOrder.quantity, restingSellOrder.quantity);
                double tradePrice = restingSellOrder.price;

                Trade trade(incomingBuyOrder.orderId, restingSellOrder.orderId, tradePrice, tradeQuantity);
                trades.push_back(trade);
                std::cout << "  -> TRADE! ";
                trade.print(); // Print trade details
                std::cout << std::endl; // Add newline after trade print
                matched = true;

                incomingBuyOrder.quantity -= tradeQuantity;
                restingSellOrder.quantity -= tradeQuantity;

                if (restingSellOrder.quantity <= 0) {
                     std::cout << "  Resting order " << restingSellOrder.orderId << " filled and removed." << std::endl;
                    it = bestAskOrders.erase(it);
                } else {
                    ++it;
                }
            }

            if (bestAskOrders.empty()) {
                book.cleanEmptyPriceLevel(OrderSide::SELL, bestAskPrice);
            }

             if (incomingBuyOrder.type == OrderType::LIMIT && incomingBuyOrder.quantity > 0 && !book.asksEmpty() && incomingBuyOrder.price < book.getBestAsk()) {
                 break;
             }
        }
        return matched;
    }

    // Uses global OrderList type now
    bool matchSellOrder(Order& incomingSellOrder) {
         bool matched = false;
        while (incomingSellOrder.quantity > 0 && !book.bidsEmpty() &&
               (incomingSellOrder.type == OrderType::MARKET || incomingSellOrder.price <= book.getBestBid()))
        {
            double bestBidPrice = book.getBestBid();
            OrderList& bestBidOrders = book.getBestBidOrders(); // This now works

            auto it = bestBidOrders.begin();
            while (it != bestBidOrders.end() && incomingSellOrder.quantity > 0) {
                Order& restingBuyOrder = *it;
                int tradeQuantity = std::min(incomingSellOrder.quantity, restingBuyOrder.quantity);
                double tradePrice = restingBuyOrder.price;

                Trade trade(incomingSellOrder.orderId, restingBuyOrder.orderId, tradePrice, tradeQuantity);
                trades.push_back(trade);
                 std::cout << "  -> TRADE! ";
                 trade.print(); // Print trade details
                 std::cout << std::endl; // Add newline after trade print
                 matched = true;

                incomingSellOrder.quantity -= tradeQuantity;
                restingBuyOrder.quantity -= tradeQuantity;

                if (restingBuyOrder.quantity <= 0) {
                     std::cout << "  Resting order " << restingBuyOrder.orderId << " filled and removed." << std::endl;
                    it = bestBidOrders.erase(it);
                } else {
                    ++it;
                }
            }

            if (bestBidOrders.empty()) {
                book.cleanEmptyPriceLevel(OrderSide::BUY, bestBidPrice);
            }

             if (incomingSellOrder.type == OrderType::LIMIT && incomingSellOrder.quantity > 0 && !book.bidsEmpty() && incomingSellOrder.price > book.getBestBid()) {
                 break;
             }
        }
        return matched;
    }
};


// --- Order Generator Function (remains the same) ---
std::vector<Order> generateSampleOrders(int numberOfOrders) {
    std::vector<Order> orders;
    // std::mt19937 rng(12345); // Fixed seed for testing
     std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count()); // Random seed
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> typeDist(0, 4); // ~20% market
    std::uniform_real_distribution<double> priceDist(99.50, 100.50);
    std::uniform_int_distribution<int> qtyDist(10, 50);
    long long currentOrderId = 1000;

    orders.reserve(numberOfOrders);
    for (int i = 0; i < numberOfOrders; ++i) {
        OrderSide side = (sideDist(rng) == 0) ? OrderSide::BUY : OrderSide::SELL;
        OrderType type = (typeDist(rng) == 0) ? OrderType::MARKET : OrderType::LIMIT;
        double price = (type == OrderType::LIMIT) ? round(priceDist(rng) * 100.0) / 100.0 : 0.0;
        int quantity = qtyDist(rng);
        orders.emplace_back(currentOrderId++, side, type, price, quantity);
    }
    return orders;
}

// --- Main Function (remains the same) ---
int main() {
    OrderBook book;
    std::vector<Trade> tradeLog;
    MatchingEngine engine(book, tradeLog);

    std::cout << "--- Generating Sample Orders ---" << std::endl;
    int numOrdersToGenerate = 25;
    std::vector<Order> incomingOrders = generateSampleOrders(numOrdersToGenerate);

    std::cout << "\n--- Processing Orders with Matching Engine ---" << std::endl;

    for (Order& order : incomingOrders) {
        engine.processOrder(order);
        std::cout << "---------------------------------------------" << std::endl;
    }

    std::cout << "\n--- Final Order Book State ---" << std::endl;
    book.printBook();

    std::cout << "\n--- Trade Log (" << tradeLog.size() << " trades) ---" << std::endl;
    if (tradeLog.empty()) {
        std::cout << "<No trades executed>" << std::endl;
    } else {
        for (const auto& trade : tradeLog) {
            trade.print(); // Print trade details
            std::cout << std::endl; // Add newline for readability
        }
    }

    // Optional: Test the inefficient removeOrder again if needed
    // std::cout << "\n--- Testing Order Removal ---" << std::endl;
    // long long orderIdToRemove = 1005; // Choose an ID that might still be in the book
    // if (book.removeOrder(orderIdToRemove)) {
    //     std::cout << "Successfully removed order " << orderIdToRemove << std::endl;
    //     book.printBook();
    // } else {
    //     // Might fail if order 1005 was filled or was market/invalid
    //     std::cout << "Order " << orderIdToRemove << " not found or not removable." << std::endl;
    // }


    std::cout << "\n--- Finished ---" << std::endl;

    return 0;
}
