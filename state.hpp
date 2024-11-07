#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>




// line and id is for the train
// status: 0 when travelling in a link, 1 when in holding area, 2 when in platform
// if status is 1 or 2, dest_platform_id is irrelevant
// this is to store the state of the train, when we have to print things out

// to save the state, you need the line which is either 'g', 'y', 'b'
// the train id, the src_platform_id, dest_platform_id, the status and the tick

struct State {
    char line;
    int id;
    int src_platform_id;
    int dest_platform_id;
    int status;
    int tick;
};

std::string link_state_to_string(State& state, const std::vector<std::string>& station_id_to_string) {
    std::string out = "";
    out += state.line;
    out += std::to_string(state.id);
    out += '-';
    out += station_id_to_string[state.src_platform_id];
    out += "->";
    out += station_id_to_string[state.dest_platform_id];
    return out;
}

std::string holding_state_to_string(State& state, const std::vector<std::string>& station_id_to_string) {
    std::string out = "";
    out += state.line;
    out += std::to_string(state.id);
    out += '-';
    out += station_id_to_string[state.src_platform_id];
    out += '#';
    return out;
}

std::string platform_state_to_string(State& state, const std::vector<std::string>& station_id_to_string) {
    std::string out = "";
    out += state.line;
    out += std::to_string(state.id);
    out += '-';
    out += station_id_to_string[state.src_platform_id];
    out += '%';
    return out;
}



std::string state_to_string(State& state, const std::vector<std::string>& station_id_to_string) {
    int status = state.status;
    if (status == 0) {
        return link_state_to_string(state, station_id_to_string);
    } else if (status == 1) {
        return holding_state_to_string(state, station_id_to_string);
    } else {
        return platform_state_to_string(state, station_id_to_string);
    }
}

void print_all_states(std::vector<State>& all_states, int num_ticks_to_print, int ticks, const std::vector<std::string>& station_id_to_string) {
    // to improve I/O speed, but do not mix C I/O (e.g. printf, scanf)
    // if have any errors, maybe try to comment out this lines
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);


    std::vector<std::vector<State>> bins(num_ticks_to_print, std::vector<State>());
    int begin = ticks - num_ticks_to_print;

    // start binning all the states
    for (State& state : all_states) {
        bins[state.tick - begin].push_back(state);
    }

    for (int i = begin; i < ticks; i ++) {
        std::vector<State>& state_at_tick = bins[i - begin];

        // collect all the string results
        std::vector<std::string> store;
        for (State& state : state_at_tick) {
            store.push_back(state_to_string(state, station_id_to_string));
        }

        // sort them in lexicographical order
        std::sort(store.begin(), store.end());
        std::cout << i << ":";
        for (std::string& str :store) {
            std::cout << " " << str;
        }
        std::cout << '\n';
    }
}

void print_all_states_ptr(State* states, int size, int num_ticks_to_print, int ticks, const std::vector<std::string>& station_id_to_string) {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    std::vector<std::vector<State>> bins(num_ticks_to_print, std::vector<State>());
    int begin = ticks - num_ticks_to_print;

    for (int i = 0; i < size; i ++) {
        State& state = states[i];
        bins[state.tick - begin].push_back(state);
    }

    for (int i = begin; i < ticks; i ++) {
        std::vector<State>& state_at_tick = bins[i - begin];

        // collect all the string results
        std::vector<std::string> store;
        for (State& state : state_at_tick) {
            store.push_back(state_to_string(state, station_id_to_string));
        }

        // sort them in lexicographical order
        std::sort(store.begin(), store.end());
        std::cout << i << ":";
        for (std::string& str :store) {
            std::cout << " " << str;
        }
        std::cout << '\n';
    }

}






