#include <unordered_map>
#include <queue>
#include <vector>
#include <optional>
#include "platform_load_time_gen.hpp"

// t is either time or platform id





struct Train {
    char line = 'z';
    int id = -1; // if -1, means train does not exist
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


struct Link {
    int travel_time = 0;
    std::optional<Train> train;
    int enter_time = 0;
    
    bool is_link_free() {
        return !train.has_value();
    }

    bool can_train_leave(int tick) {
        return train.has_value() && enter_time + travel_time == tick;
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
    PlatformLoadTimeGen pltg;
    std::unordered_map<char, bool> which_lines;
    std::unordered_map<char, int> output_platforms;
    std::vector<int> input_platforms;
    std::priority_queue<Pair, std::vector<Pair>, Compare> pq;
    int terminal; // 0 if is start, 1 if it 
    std::vector<Train> trains_to_spawn;

    Link link;
    std::optional<Train> train;
    int unloading_time = 0;
    int enter_time = 0;

    Platform(): pltg(1) {}

    Platform(int popularity): pltg(popularity) {}
    
    bool is_platform_free() {
        return !train.has_value();
    }

    bool can_train_leave(int tick) {
        return train.has_value() && enter_time + unloading_time == tick;
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
        
        if (link.can_train_leave(tick)) {
            out = link.train_leave();
        }

        if (can_train_leave(tick)) {
            link.train_enter(train.value(), tick);
            train.reset();
        }

        int dest_platform = (out.id == -1) ? 'b' : output_platforms[out.line];
        return {out, dest_platform};
    }

    void send_in(std::vector<Train> &trains, int tick) {
        for (Train &t : trains) pq.push({t, tick});
    }

    void push_train_to_platform(int tick) {
        if (!pq.empty() && is_platform_free()) {
            Pair p = pq.top(); pq.pop();
            train_enter(p.train, tick);
        }
    }
};