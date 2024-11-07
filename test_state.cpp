#include "state.hpp"
#include <bits/stdc++.h>
using namespace std;

vector<string> station_ids_to_string = {"changi", "tampines", "clementi"};

void test1() {
    State s = {'g', 1, 0, 1, 0, 100};
    string ans = link_state_to_string(s, station_ids_to_string);
    string expected = "g1-changi->tampines";
    //cout << ans << endl;
    if (ans != expected) throw runtime_error(ans);
}

void test2() {
    State s = {'b', 0, 0, 1, 1, 100};
    string ans = holding_state_to_string(s, station_ids_to_string);
    string expected = "b0-changi#";
    //cout << ans << endl;
    if (ans != expected) throw runtime_error(ans);
}

void test3() {
    State s = {'y', 12, 2, 0, 2, 100};
    string ans = platform_state_to_string(s, station_ids_to_string);
    string expected = "y12-clementi%";
    //cout << ans << endl;
    if (ans != expected) throw runtime_error(ans);
}

void test4() {
    vector<State> states = {{'g', 0, 1, 2, 0, 0}, 
                            {'g', 1, 1, 2, 2, 1},
                            {'b', 1, 0, 1, 1, 1}, 
                            {'y', 2, 0, 2, 2, 2}, 
                            {'b', 10, 0, 2, 0, 0}};
    print_all_states(states, 3, 3, station_ids_to_string);
}

void test5() {
    State states[5] = {{'g', 0, 1, 2, 0, 0}, 
                       {'g', 1, 1, 2, 2, 1},
                       {'b', 1, 0, 1, 1, 1}, 
                       {'y', 2, 0, 2, 2, 2}, 
                       {'b', 10, 0, 2, 0, 0}};
    print_all_states_ptr(states, 5, 3, 3, station_ids_to_string);
}

int main() {
    //test1();
    //test2();
    //test3();
    //test4();
    test5();
}