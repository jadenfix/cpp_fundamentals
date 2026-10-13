#define main sample_demo_main
#include "../fundamentals.cpp"
#undef main

#include <cstdlib>

#define CHECK(condition) do { if (!(condition)) { \
    std::cerr << "Failed: " #condition << " at line " << __LINE__ << '\n'; \
    std::exit(1); \
} } while (false)

int main() {
    OrderBook book;
    std::vector<Trade> trades;
    MatchingEngine engine(book, trades, true);

    Order first(41, OrderSide::SELL, OrderType::LIMIT, 100.0, 5);
    CHECK(engine.processOrder(first));
    CHECK(book.getBestAskOrders().size() == 1);

    Order duplicate(41, OrderSide::SELL, OrderType::LIMIT, 99.0, 9);
    CHECK(!engine.processOrder(duplicate));
    CHECK(duplicate.quantity == 9);
    CHECK(book.getBestAsk() == 100.0);
    CHECK(book.getBestAskOrders().size() == 1);

    Order zeroId(0, OrderSide::BUY, OrderType::MARKET, 0.0, 3);
    Order negativeId(-7, OrderSide::BUY, OrderType::LIMIT, 101.0, 3);
    CHECK(!engine.processOrder(zeroId));
    CHECK(!engine.processOrder(negativeId));
    CHECK(trades.empty());

    Order fill(42, OrderSide::BUY, OrderType::MARKET, 0.0, 5);
    CHECK(engine.processOrder(fill));
    CHECK(fill.quantity == 0 && trades.size() == 1 && book.asksEmpty());

    Order reusedAfterFill(41, OrderSide::BUY, OrderType::LIMIT, 99.0, 4);
    CHECK(!engine.processOrder(reusedAfterFill));
    CHECK(book.bidsEmpty());

    Order invalidQuantity(43, OrderSide::BUY, OrderType::LIMIT, 99.0, 0);
    CHECK(!engine.processOrder(invalidQuantity));
    Order corrected(43, OrderSide::BUY, OrderType::LIMIT, 99.0, 4);
    CHECK(engine.processOrder(corrected));
    CHECK(book.getBestBidOrders().front().orderId == 43);

    std::cout << "Order identity regression checks passed.\n";
}
