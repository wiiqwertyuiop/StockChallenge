// Christian Chernock

#include <set>
#include <iostream>
#include <string>
#include <map>
#include <tuple> // These are just used to make input data easier
#include <vector> //

class Firm {
    int liveOrders;
    int filledOrders;
    float totalBalance;

public:
    Firm() {
        this->totalBalance = 0;
        this->filledOrders = 0;
        this->liveOrders = 0;
    }

    void newOrder() {
        this->liveOrders++;
    }

    void cancelOrder() {
        this->liveOrders--;
    }

    void completeOrder(float amount) {
        this->totalBalance += amount;
        this->filledOrders++;
        this->liveOrders--;
    }

    float getBalance() {
        return totalBalance;
    }
    
    std::string toString() {
        return std::to_string(liveOrders) + " " + std::to_string(filledOrders);
    }
};

class Order {
    uint16_t ID;
    std::string symbol;
    float price;
    char side;

public:
    Order() {}

    Order(uint16_t firmId, std::string symbol) {
        this->ID = firmId;
        this->symbol = symbol;
    }

    Order(uint16_t firmId, std::string symbol, char side, float price) {
        this->ID = firmId;
        this->price = price;
        this->side = side;
        this->symbol = symbol;
    }

    uint16_t getWho() {
        return this->ID;
    }

    void setPrice(float price) {
        this->price = price;
    }

    float getPrice() {
        return this->price;
    }

    std::string getSymbol() {
        return this->symbol;
    }

    char getSide() {
        return this->side;
    }

    bool operator< (const Order& _s) const {
        if (_s.ID == this->ID) return _s.symbol < this->symbol;
        return _s.ID < this->ID;
    }
};

class MatchingEngine
{
    // the orderQueue is a master list of all orders; it connects orders with their firms 
    // the firms keep track of their own stats, and how many active orders they have
    std::map<uint16_t, Firm> FirmList;
    std::set<Order> orderQueue;
    
    Firm& getFirm(uint16_t firmId) {
        if (!FirmList.count(firmId)) {
            FirmList.insert(std::pair<uint16_t, Firm>(firmId, Firm()));
        }
        return FirmList[firmId];
    }
    
public:
    void onNewOrder(uint16_t firmId, std::string symbol, char side, float price) {
        // If the attempted order already exists, return
        if (orderQueue.count(Order(firmId, symbol))) return;
        getFirm(firmId).newOrder();
        
        // If this is a buy/sell, check if there is an equivilant sell/buy
        std::set<Order>::iterator it;
        for (it = orderQueue.begin(); it != orderQueue.end(); it++) {
            Order curOrder = *it;
            if (curOrder.getSymbol() == symbol && curOrder.getSide() != side) break;
        }
        
        // If there is no other buy/sell for this symbol
        // then there is nothing else to do; add it to the queue and return
        if (it == orderQueue.end()) {
            orderQueue.insert(Order(firmId, symbol, side, price));
            return;
        }
        
        // We need to flip things depending on 
        // if we are entering as the buyer or the seller
        int flip = side == 'B' ? -1 : 1;
        
        // If the sell price is higher than the buy price we can't match 
        // if thats the case queue it and return
        Order previousOrder = *it;
        if (previousOrder.getPrice()*flip < price*flip) {
            orderQueue.insert(Order(firmId, symbol, side, price));
            return;
        }
        
        // We found a match
        getFirm(firmId).completeOrder(price * flip);
        getFirm(previousOrder.getWho()).completeOrder(-price * flip);
        orderQueue.erase(it);
    }

    void onModify(uint16_t firmId, std::string symbol, float price) {
        // Loop until we find matching order
        for (std::set<Order>::iterator it = orderQueue.begin(); it != orderQueue.end(); it++) {
            Order curOrder = *it;
            if (curOrder.getWho() == firmId && curOrder.getSymbol() == symbol) {
                // If we found one, erase it and post modified order
                char side = curOrder.getSide();
                orderQueue.erase(it);
                getFirm(firmId).cancelOrder();
                onNewOrder(firmId, symbol, side, price);
                return;
            }
        }
    }

    void onCancel(uint16_t firmId, std::string symbol) {
        // Loop until we find matching order
        for (std::set<Order>::iterator it = orderQueue.begin(); it != orderQueue.end(); it++) {
            Order curOrder = *it;
            if (curOrder.getWho() == firmId && curOrder.getSymbol() == symbol) {
                getFirm(firmId).cancelOrder();
                orderQueue.erase(it);
                return;
            }
        }
    }

    void print() {
        for (auto const& curFirm : FirmList) {
            Firm firm = curFirm.second;
            std::cout << curFirm.first << " " << firm.toString() << " " << firm.getBalance() << "\n";
        }
    }
};

int main() {
    
    MatchingEngine me;
    
    // Tuple: OrderType, Firm ID, Symbol, Side, Price
    using Tuple = std::tuple<char, uint16_t, std::string, char, float>;
    std::vector<Tuple> msgStream = {
        /*Tuple('N', 1001, "APPL", 'S', 250.0),
        Tuple('N', 99, "APPL", 'B', 251.0),*/
        /*Tuple('N', 1001, "CARB", 'S', 250.51),
        Tuple('N', 1000, "BEAN", 'B', 10.00),
        Tuple('N', 1001, "BEAN", 'B', 9.99),
        Tuple('C', 1001, "BEAN", NULL, NULL),
        Tuple('N', 1001, "BEAN", 'S', 10.00)*/
        Tuple('N', 1738, "APPL", 'B', 1500.50),
        Tuple('N', 1738, "CME", 'S', 500.50),
        Tuple('N', 2001, "APPL", 'S', 1500.51),
        Tuple('N', 1738, "VIRT", 'B', 100.35),
        Tuple('N', 2022, "APPL", 'S', 1500.49),
        Tuple('M', 2001, "APPL", NULL, 1500.48),
        Tuple('C', 2001, "APPL", NULL, NULL),
        Tuple('C', 2001, "CME", NULL, NULL),
        Tuple('N', 2023, "NTFLX", 'S', 15.00),
        Tuple('N', 1000, "NTFLX", 'B', 10.00),
        Tuple('M', 2023, "NTFLX", NULL, 10.00),
        Tuple('N', 2023, "CME", 'B', 500.51),
    };
    
    for (Tuple msg : msgStream) {
        switch (std::get<0>(msg)) {
            case 'N':
                me.onNewOrder(std::get<1>(msg), std::get<2>(msg), std::get<3>(msg), std::get<4>(msg));
                break;
            case 'M':
                me.onModify(std::get<1>(msg), std::get<2>(msg), std::get<4>(msg));
                break;            
            case 'C':
                me.onCancel(std::get<1>(msg), std::get<2>(msg));
                break;
            default: 
                break;
        }
    }

    std::cout << "Output:\n";
    me.print();
}
