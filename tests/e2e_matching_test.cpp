#define main sample_demo_main
#include "../fundamentals.cpp"
#undef main

#include <cstdlib>

#define CHECK(condition) do { if (!(condition)) { \
    std::cerr << "Failed: " #condition << " at line " << __LINE__ << '\n'; \
    std::exit(1); \
} } while (false)

void invalidPricesCannotTrade() {
    for (double price : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()}) {
        for (OrderSide side : {OrderSide::BUY, OrderSide::SELL}) {
            OrderBook book;
            std::vector<Trade> trades;
            MatchingEngine engine(book, trades);
            book.addOrder(Order(1, OrderSide::BUY, OrderType::LIMIT, 100, 10));
            book.addOrder(Order(2, OrderSide::SELL, OrderType::LIMIT, 101, 10));
            Order invalid(3, side, OrderType::LIMIT, price, 5);
            engine.processOrder(invalid);
            book.addOrder(invalid);
            CHECK(trades.empty());
            CHECK(invalid.quantity == 5);
            CHECK(book.getBestBid() == 100 && book.getBestAsk() == 101);
            CHECK(book.getBestBidOrders().size() == 1);
            CHECK(book.getBestAskOrders().size() == 1);
        }
    }
}

void priceTimePriorityAndResiduals() {
    for (OrderSide incomingSide : {OrderSide::BUY, OrderSide::SELL}) {
        OrderBook book;
        std::vector<Trade> trades;
        MatchingEngine engine(book, trades);
        const OrderSide restingSide = incomingSide == OrderSide::BUY ? OrderSide::SELL : OrderSide::BUY;
        const double best = incomingSide == OrderSide::BUY ? 100 : 101;
        const double next = incomingSide == OrderSide::BUY ? 101 : 100;
        book.addOrder(Order(1, restingSide, OrderType::LIMIT, next, 7));
        book.addOrder(Order(2, restingSide, OrderType::LIMIT, best, 3));
        book.addOrder(Order(3, restingSide, OrderType::LIMIT, best, 4));
        Order crossing(4, incomingSide, OrderType::LIMIT, best, 9);
        engine.processOrder(crossing);
        CHECK(trades.size() == 2);
        CHECK(trades[0].restingOrderId == 2 && trades[0].quantity == 3);
        CHECK(trades[1].restingOrderId == 3 && trades[1].quantity == 4);
        CHECK(trades[0].price == best && trades[1].price == best);
        CHECK(crossing.quantity == 2);
        CHECK(book.getBestBid() < book.getBestAsk());
        CHECK(book.removeOrder(4));
        Order sweep(5, incomingSide, OrderType::MARKET, 0, 10);
        engine.processOrder(sweep);
        CHECK(trades.size() == 3 && trades[2].restingOrderId == 1);
        CHECK(trades[2].quantity == 7 && sweep.quantity == 3);
        CHECK(book.bidsEmpty() && book.asksEmpty());
    }
}

void partialFillAndCancellation() {
    OrderBook book;
    std::vector<Trade> trades;
    MatchingEngine engine(book, trades);
    book.addOrder(Order(1, OrderSide::SELL, OrderType::LIMIT, 100, 10));
    Order buy(2, OrderSide::BUY, OrderType::MARKET, 0, 4);
    engine.processOrder(buy);
    CHECK(buy.quantity == 0 && trades.size() == 1);
    CHECK(book.getBestAskOrders().front().quantity == 6);
    CHECK(book.removeOrder(1) && book.asksEmpty());
    Order zero(3, OrderSide::BUY, OrderType::LIMIT, 200, 0);
    engine.processOrder(zero);
    CHECK(book.bidsEmpty() && trades.size() == 1);
}

int main() {
    invalidPricesCannotTrade();
    priceTimePriorityAndResiduals();
    partialFillAndCancellation();
    std::cout << "All matching-engine regression checks passed.\n";
}
