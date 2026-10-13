#define main sample_demo_main
#include "fundamentals.cpp"
#undef main
#include <cassert>

int main() {
    OrderBook book;
    std::vector<Trade> trades;
    MatchingEngine engine(book, trades);
    Order first(1, OrderSide::SELL, OrderType::LIMIT, 100.0, 3);
    Order second(2, OrderSide::SELL, OrderType::LIMIT, 100.0, 4);
    engine.processOrder(first);
    engine.processOrder(second);
    Order invalid(3, OrderSide::SELL, OrderType::LIMIT, -1.0, 5);
    engine.processOrder(invalid);
    Order nan(4, OrderSide::BUY, OrderType::LIMIT, std::numeric_limits<double>::quiet_NaN(), 5);
    engine.processOrder(nan);
    assert(trades.empty());
    Order buy(5, OrderSide::BUY, OrderType::LIMIT, 100.0, 5);
    engine.processOrder(buy);
    assert(trades.size() == 2);
    assert(trades[0].restingOrderId == 1 && trades[0].quantity == 3);
    assert(trades[1].restingOrderId == 2 && trades[1].quantity == 2);
    assert(buy.quantity == 0);
    assert(book.getBestAsk() == 100.0);
    Order market(6, OrderSide::BUY, OrderType::MARKET, 0.0, 4);
    engine.processOrder(market);
    assert(trades.size() == 3 && trades.back().quantity == 2);
    assert(market.quantity == 2 && book.asksEmpty());
    assert(book.removeOrder(2) == false);
    Order rest(7, OrderSide::BUY, OrderType::LIMIT, 99.0, 2);
    engine.processOrder(rest);
    assert(book.removeOrder(7) && book.bidsEmpty());
    std::cout << "PASS: FIFO, partial fills, market exhaustion, cancellation and invalid prices\n";
}
