//
// Created by david on 09/06/23.
//

#ifndef MATCHING_ENGINE_ORDERBOOK_H
#define MATCHING_ENGINE_ORDERBOOK_H

#include <memory>
#include <vector>

enum OrderSide {
    BUY, SELL
};

struct OrderStruct {
    OrderSide side;
    int id{};
    short price{};
    int quantity{};

    int compare_by{};

    explicit OrderStruct(const std::string &line);

    std::string to_string(bool reversed_items);

    virtual int display_volume() = 0;
};

typedef std::shared_ptr<OrderStruct> Order;

struct LimitOrder : public OrderStruct {
    explicit LimitOrder(const std::string &line);

    int display_volume() override;
};

struct IcebergOrder : public OrderStruct {
    int peak;

    explicit IcebergOrder(const std::string &line);

    int display_volume() override;
};

class OrderBook {
    std::vector<Order> sell_orders, buy_orders;

    static void insert_into(std::vector<Order> &orders, const Order &order);

public:
    void insert(const Order &order);

    void print_order_book();

    void match(const Order &incoming_order);

    void process();
};

#endif //MATCHING_ENGINE_ORDERBOOK_H
