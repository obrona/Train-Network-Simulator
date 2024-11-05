#include <unordered_map>
#include <queue>
#include <vector>
#include <optional>
#include <algorithm>

#include "platform_load_time_gen.hpp"
#include "state.hpp"

using std::make_heap;
using std::push_heap;
using std::pop_heap;

struct Train {
    char line = 'z';
    int id = -1; // if -1, means train does not exist

    bool operator==(const Train& other) {
        return other.line == line && other.id == id;
    }
};

// t is the timestamp
struct Pair {
    Train train;
    int t;
};

struct Compare {
    bool operator()(const Pair& a, const Pair& b) {
        return (a.t == b.t) ? a.train.id > b.train.id : a.t > b.t;
    }
};

const Compare compare;



struct Link {
    int travel_time = 0;
    std::optional<Train> train;
    int enter_time = 0;

    Link() {}

    Link(int travel_time): travel_time(travel_time) {}
    
    bool is_link_free() {
        return !train.has_value();
    }

    bool can_train_leave(int tick) {
        return train.has_value() && enter_time + travel_time >= tick;
    } 

    Train train_leave() {
        Train out = train.value();
        train.reset();
        return out;
    }

    void train_enter(Train& train, int tick) {
        this->train = train;
        enter_time = tick;
    }
};



struct Platform {
    int src_station_id, dest_station_id;
    PlatformLoadTimeGen pltg;
    std::unordered_map<char, int> output_platforms;
    std::vector<int> input_platforms;

    std::vector<Pair> pq;
    std::vector<State> saved_states;

    Link link;
    std::optional<Train> train;
    int unloading_time = 0;
    int enter_time = 0;

    Platform(): pltg(1) {}

    Platform(int popularity): pltg(popularity) {}

    Platform(int popularity, int link_travel_time): pltg(popularity), link(link_travel_time) {}

    Platform(int src_station_id, int dest_station_id, int popularity, int link_travel_time): 
        src_station_id(src_station_id), 
        dest_station_id(dest_station_id), 
        pltg(popularity),
        link(link_travel_time) {}
    
    bool is_platform_free() {
        return !train.has_value();
    }

    bool can_train_leave(int tick) {
        return train.has_value() && enter_time + unloading_time >= tick;
    }

    Train train_leave() {
        Train out = train.value();
        train.reset();
        return out;
    }

    void train_enter(Train train, int tick) {
        this->train = train;
        unloading_time = pltg.next(train.id);
        enter_time = tick;
    }


    // 1. call send_out first
    // 2. then send in all the arriving trainsd
    // 3. then push train to platform, if possible

    // return the train that leaves the outgoing link that is paired with this platform
    // and the platform_id 
    Pair send_out(int tick) {
        Train out = {'b', -1};
        
        // check if train can leave link
        if (link.can_train_leave(tick)) {
            out = link.train_leave();
        }

        // check whether we cn puh train from platform to link. It requires train to finish unloading, and link to be free
        if (link.is_link_free() && can_train_leave(tick)) {
            link.train_enter(train.value(), tick);
            train.reset();
        }

        int dest_platform = (out.id == -1) ? 'b' : output_platforms[out.line];
        return {out, dest_platform};
    }

    void send_in(std::vector<Train> &trains, int tick) {
        for (Train &t : trains) {
            pq.push_back({t, tick});
            push_heap(pq.begin(), pq.end(), compare);
        }
    }

    void send_in(Train train, int tick) {
        pq.push_back({train, tick});
        push_heap(pq.begin(), pq.end(), compare);
    }

    void push_train_to_platform(int tick) {
        if (!pq.empty() && is_platform_free()) {
            pop_heap(pq.begin(), pq.end(), compare);
            train_enter(pq.back().train, tick);
            pq.pop_back();
        }
    }

    void save_train_in_link_state(int tick) {
        if (link.is_link_free()) return;
        saved_states.push_back({link.train.value().line, 
                                link.train.value().id,
                                src_station_id,
                                dest_station_id,
                                0,
                                tick});

    }

    void save_train_in_platform_state(int tick) {
        if (is_platform_free()) return;
        saved_states.push_back({train.value().line,
                                train.value().id,
                                src_station_id,
                                dest_station_id,
                                2,
                                tick});
    }

    void save_all_trains_in_holding_state(int tick) {
        for (Pair& p : pq) {
            Train& t = p.train;
            saved_states.push_back({t.line, t.id, src_station_id, dest_station_id, 1, tick});
        }
    }

    // call this function to save all states in the save_state vector
    // only call this after you have done all the updates for this tick
    void save_all_states(int tick) {
        save_train_in_link_state(tick);
        save_train_in_platform_state(tick);
        save_all_trains_in_holding_state(tick);
    }
};