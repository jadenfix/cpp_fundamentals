#define main sample_demo_main
#include "../fundamentals.cpp"
#undef main
#include <cassert>

int main() {
    OrderBook book;
    std::vector<Trade> trades;
    MatchingEngine engine(book, trades);
    auto submit = [&](long long id, OrderSide side, OrderType type, double price, int quantity) {
        Order order(id, side, type, price, quantity);
        engine.processOrder(order);
        return order;
    };
    submit(1, OrderSide::SELL, OrderType::LIMIT, 100, 5);
    submit(2, OrderSide::SELL, OrderType::LIMIT, 100, 7);
    submit(3, OrderSide::SELL, OrderType::LIMIT, 101, 9);
    auto buy = submit(4, OrderSide::BUY, OrderType::LIMIT, 100, 8);
    assert(buy.quantity == 0 && trades.size() == 2);
    assert(trades[0].restingOrderId == 1 && trades[0].quantity == 5);
    assert(trades[1].restingOrderId == 2 && trades[1].quantity == 3);
    assert(trades[0].price == 100 && trades[1].price == 100);
    assert(book.getBestAskOrders().front().quantity == 4);
    auto sweep = submit(5, OrderSide::BUY, OrderType::MARKET, 0, 20);
    assert(sweep.quantity == 7 && trades.size() == 4 && book.asksEmpty());
    assert(book.bidsEmpty()); // Market remainder never rests.
    assert(trades[2].price == 100 && trades[3].price == 101);

    submit(6, OrderSide::BUY, OrderType::LIMIT, 99, 10);
    for (double price : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()}) {
        submit(7, OrderSide::SELL, OrderType::LIMIT, price, 3);
        book.addOrder(Order(8, OrderSide::SELL, OrderType::LIMIT, price, 3));
        assert(trades.size() == 4 && book.asksEmpty());
        assert(book.getBestBidOrders().front().quantity == 10);
    }
    submit(9, OrderSide::SELL, OrderType::LIMIT, 99, 0);
    submit(10, OrderSide::SELL, OrderType::LIMIT, 99, -1);
    assert(trades.size() == 4);
    auto sell = submit(11, OrderSide::SELL, OrderType::LIMIT, 98, 12);
    assert(sell.quantity == 2 && trades.size() == 5);
    assert(trades.back().price == 99 && trades.back().quantity == 10);
    assert(book.bidsEmpty() && book.getBestAsk() == 98);
    assert(book.removeOrder(11) && book.asksEmpty());
    assert(!book.removeOrder(11));
    bool empty_access_throws = false;
    try { book.getBestAskOrders(); } catch (const std::runtime_error&) { empty_access_throws = true; }
    assert(empty_access_throws);
    assert(submit(12, OrderSide::BUY, OrderType::MARKET, 0, 4).quantity == 4);
    assert(trades.size() == 5 && book.bidsEmpty() && book.asksEmpty());
}
