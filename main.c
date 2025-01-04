module main;

import * from "std:io"; # import all

contract action {
    bool make();
};

internal struct kernel {
    internal int time = 3; # time before it pops
};

struct popcorn : kernel {
    float temp;
} action;

# constructor
popcorn : popcorn(float temp, int time) {
    this->time = time;
    this->temp = temp;
}

# destructor
popcorn : %popcorn() {}

# contract fulfillment
popcorn : bool make() {
    if (this->temp >= 180.0) {
        return true;     # popped successfully
    } else {
        return false;    # not popped yet
    }
}

popcorn *pcorn = new popcorn(200.0, 5);
bool completed = pcorn->make();

print(pcorn->temp);
printn(completed);  # prints if popped (true/false)

purge pcorn;
