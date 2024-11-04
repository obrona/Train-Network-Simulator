#include "state.hpp"
#include <bits/stdc++.h>
using namespace std;

vector<string> platform_ids_to_string = {"changi", "tampines", "clementi"};

void test1() {
    State s = {'g', 1, 0, 1, 0};
    string ans = link_state_to_string(s, platform_ids_to_string);
    string expected = "g1-changi->tampines";
    //cout << ans << endl;
    if (ans != expected) throw runtime_error(ans);
}

void test2() {
    State s = {'b', 0, 0, 1, 1};
    string ans = holding_state_to_string(s, platform_ids_to_string);
    string expected = "b0-changi#";
    //cout << ans << endl;
    if (ans != expected) throw runtime_error(ans);
}

void test3() {
    State s = {'y', 12, 2, 0, 2};
    string ans = platform_state_to_string(s, platform_ids_to_string);
    string expected = "y12-clementi%";
    //cout << ans << endl;
    if (ans != expected) throw runtime_error(ans);
}


int main() {
    test1();
    test2();
    test3();
}