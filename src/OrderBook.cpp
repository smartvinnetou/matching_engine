//
// Created by david on 09/06/23.
//
#include <iostream>
#include <string>
#include <memory>
#include <sstream>
#include <iomanip>

#include "OrderBook.h"

// HELPER FUNCTIONS

const char INPUT_SEPARATOR = ',';
const char OUTPUT_SEPARATOR = '|';
const char TRADE_OUTPUT_SEPARATOR = ',';
const std::string BUY_EMPTY_LINE = "|          |             |       |";
const std::string SELL_EMPTY_LINE = "|       |             |          |";

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

// instantiate the right Order class according to the message in the input line
Order order_from_string(std::string line) {
    auto number_of_fields = std::count(line.begin(), line.end(), INPUT_SEPARATOR) + 1;
    if (number_of_fields == 4) {
        return Order(new LimitOrder(line));
    } else {
        // assuming perfectly well-formed input, the only other option is iceberg order
        return Order(new IcebergOrder(line));
    }
}

// class for creating locale that uses comma as thousands separator in numbers
class use_thousands_separator : public std::numpunct<char> {
protected:
    char do_thousands_sep() const override {
        return ',';
    }

    std::string do_grouping() const override {
        return "\03";
    }
};

// METHODS

// OrderStruct
//-----------------

OrderStruct::OrderStruct(const std::string &line) {
    std::string field;
    std::istringstream iss(line);

    // get order side
    std::getline(iss, field, INPUT_SEPARATOR);
    if (line[0] == 'S') {
        this->side = SELL;
    } else {
        // assuming well-formed input
        this->side = BUY;
    }

    // get order id
    std::getline(iss, field, INPUT_SEPARATOR);
    this->id = std::stoi(field);

    // get price
    std::getline(iss, field, INPUT_SEPARATOR);
    this->price = static_cast<short>(std::stoi(field));
    if (this->side == SELL) {
        this->compare_by = this->price;
    } else {
        this->compare_by = -1 * this->price;
    }

    // get quantity
    std::getline(iss, field, INPUT_SEPARATOR);
    this->quantity = std::stoi(field);
}

std::string OrderStruct::to_string(bool reversed_items) {
    std::locale comma_locale(std::locale(), new use_thousands_separator());
    std::ostringstream output_string;

    if (reversed_items) {
        // right adjusted, 7 characters fixed length
        output_string.imbue(comma_locale);
        output_string << std::right << std::setw(7) << this->price << OUTPUT_SEPARATOR;
        // right adjusted, 13 characters fixed length
        output_string.imbue(comma_locale);
        output_string << std::right << std::setw(13) << this->display_volume() << OUTPUT_SEPARATOR;
        // right adjusted, 10 characters fixed length
        output_string.imbue(std::locale());
        output_string << std::right << std::setw(10) << this->id;
    } else {
        // right adjusted, 10 characters fixed length
        output_string << std::right << std::setw(10) << this->id << OUTPUT_SEPARATOR;
        output_string.imbue(comma_locale);
        // right adjusted, 13 characters fixed length
        output_string << std::right << std::setw(13) << this->display_volume() << OUTPUT_SEPARATOR;
        // right adjusted, 7 characters fixed length
        output_string << std::right << std::setw(7) << this->price;
        output_string.imbue(std::locale());
    }
    return output_string.str();
}

// IcebergOrder
//----------------

IcebergOrder::IcebergOrder(const std::string &line) : OrderStruct(line) {
    std::string field;
    std::istringstream iss(line);

    // skip common fields
    std::getline(iss, field, INPUT_SEPARATOR);
    std::getline(iss, field, INPUT_SEPARATOR);
    std::getline(iss, field, INPUT_SEPARATOR);
    std::getline(iss, field, INPUT_SEPARATOR);

    // get peak
    std::getline(iss, field, INPUT_SEPARATOR);
    this->peak = std::stoi(field);
}

int IcebergOrder::display_volume() {
    return std::min(this->peak, this->quantity);
}

// LimitOrder
//---------------

LimitOrder::LimitOrder(const std::string &line) : OrderStruct(line) {}

int LimitOrder::display_volume() {
    return this->quantity;
}

// OrderBook
//---------------------

void OrderBook::insert_into(std::vector<Order> &orders, const Order &order) {
    // find the place in the order book where the new order fits given its price
    auto loc = std::upper_bound(orders.begin(), orders.end(), order,
                                [](auto &lhs, auto &rhs) { return lhs->compare_by < rhs->compare_by; });
    // insert the order into the order book
    orders.insert(loc, order);
}

void OrderBook::insert(const Order &order) {
    if (order->side == SELL) {
        this->insert_into(this->sell_orders, order);
    } else {
        this->insert_into(this->buy_orders, order);
    }
}

void OrderBook::print_order_book() {
    // print the header
    std::cout << "+-----------------------------------------------------------------+" << std::endl;
    std::cout << "| BUY                            | SELL                           |" << std::endl;
    std::cout << "| Id       | Volume      | Price | Price | Volume      | Id       |" << std::endl;
    std::cout << "+----------+-------------+-------+-------+-------------+----------+" << std::endl;

    // print lines with both sides
    auto number_of_full_lines = std::min(this->sell_orders.size(), this->buy_orders.size());
    for (auto i = 0; i < number_of_full_lines; i++) {
        // print one full line
        std::cout << OUTPUT_SEPARATOR << this->buy_orders[i]->to_string(false) << OUTPUT_SEPARATOR
                  << this->sell_orders[i]->to_string(true) << OUTPUT_SEPARATOR << std::endl;
    }
    if (this->sell_orders.size() > this->buy_orders.size()) {
        // sell side of the order book is longer, print it out
        for (auto i = number_of_full_lines; i < this->sell_orders.size(); i++) {
            std::cout << BUY_EMPTY_LINE << this->sell_orders[i]->to_string(true) << OUTPUT_SEPARATOR << std::endl;
        }
    } else {
        // buy side of the order book is longer, print it out
        for (auto i = number_of_full_lines; i < this->buy_orders.size(); i++) {
            std::cout << OUTPUT_SEPARATOR << this->buy_orders[i]->to_string(false) << SELL_EMPTY_LINE << std::endl;
        }
    }

    // print out footer
    std::cout << "+-----------------------------------------------------------------+" << std::endl;
}

bool is_trade_possible(const Order &order_from_order_book, const Order &incoming_order) {
    if (incoming_order->side == BUY) {
        return order_from_order_book->price <= incoming_order->price;
    } else {
        return order_from_order_book->price >= incoming_order->price;
    }
}

void match_one_side(std::vector<Order> &order_book, const Order &incoming_order) {
    while (!(order_book.empty()) && incoming_order->quantity > 0 && is_trade_possible(order_book[0], incoming_order)) {
        Order top_order = order_book[0];
        short trade_price = top_order->price;
        int trade_quantity = std::min(incoming_order->quantity, top_order->quantity);

        // print out the trade message
        std::cout << top_order->id << TRADE_OUTPUT_SEPARATOR << incoming_order->id << TRADE_OUTPUT_SEPARATOR
                  << trade_price << TRADE_OUTPUT_SEPARATOR << trade_quantity << std::endl;
        // update the order book
        top_order->quantity -= trade_quantity;
        if (top_order->quantity <= 0) {
            // the top incoming_order is fully filled, remove it
            order_book.erase(order_book.begin());
        }
        // update the incoming incoming_order
        incoming_order->quantity -= trade_quantity;
    }
}

void OrderBook::match(const Order &incoming_order) {
    if (incoming_order->side == BUY) {
        // while we have a matching incoming_order, keep filling
        match_one_side(this->sell_orders, incoming_order);
    } else {
        // while we have a matching incoming_order, keep filling
        match_one_side(this->buy_orders, incoming_order);
    }
}

// top-level method that processes the incoming messages and invokes that OrderBook's logic

void OrderBook::process() {
    for (std::string line; std::getline(std::cin, line);) {
        trim(line);
        if ( !(line.empty()) && line[0] != '#') {
            Order incoming_order = order_from_string(line);
            this->match(incoming_order);
            if (incoming_order->quantity > 0) {
                this->insert(incoming_order);
            }
            this->print_order_book();
        }
    }
}
