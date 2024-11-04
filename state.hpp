#pragma once
#include <string>
#include <vector>



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

std::string link_state_to_string(State& state, std::vector<std::string>& platform_id_to_string) {
    std::string out = "";
    out += state.line;
    out += std::to_string(state.id);
    out += '-';
    out += platform_id_to_string[state.src_platform_id];
    out += "->";
    out += platform_id_to_string[state.dest_platform_id];
    return out;
}

std::string holding_state_to_string(State& state, std::vector<std::string>& platform_id_to_string) {
    std::string out = "";
    out += state.line;
    out += std::to_string(state.id);
    out += '-';
    out += platform_id_to_string[state.src_platform_id];
    out += '#';
    return out;
}

std::string platform_state_to_string(State& state, std::vector<std::string>& platform_id_to_string) {
    std::string out = "";
    out += state.line;
    out += std::to_string(state.id);
    out += '-';
    out += platform_id_to_string[state.src_platform_id];
    out += '%';
    return out;
}



std::string state_to_string(State& state, std::vector<std::string>& platform_id_to_string) {
    int status = state.status;
    if (status == 0) {
        return link_state_to_string(state, platform_id_to_string);
    } else if (status == 1) {
        return holding_state_to_string(state, platform_id_to_string);
    } else {
        return platform_state_to_string(state, platform_id_to_string);
    }
}






