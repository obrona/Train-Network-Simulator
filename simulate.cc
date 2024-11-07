#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <algorithm>

#include <mpi.h>

#include "structs.hpp"
#include "state.hpp"

using std::string;
using std::unordered_map;
using std::vector;
using adjacency_matrix = std::vector<std::vector<size_t>>;


// create mpi_Train type
void create_mpi_Train(MPI_Datatype *my_type) {
    int num_elements = 2;  // Number of elements in the struct
    int block_lengths[] = {1, 1};  // Number of items of each data type in the struct
    MPI_Datatype types[] = {MPI_CHAR, MPI_INT};  // Data types of each struct element
    MPI_Aint offsets[] = {offsetof(Train, line), offsetof(Train, id)};  // Offsets of each struct element
    MPI_Type_create_struct(num_elements, block_lengths, offsets, types, my_type);
    MPI_Type_commit(my_type);
}

// create mpi_State type. You need this to send back states of the train to the master node
void create_mpi_State(MPI_Datatype *my_type) {
    int num_elements = 2;  // Number of elements in the struct
    int block_lengths[] = {1, 5};  // Number of items of each data type in the struct
    MPI_Datatype types[] = {MPI_CHAR, MPI_INT};  // Data types of each struct element
    MPI_Aint offsets[] = {offsetof(State, line), offsetof(State, id)};  // Offsets of each struct element
    MPI_Type_create_struct(num_elements, block_lengths, offsets, types, my_type);
    MPI_Type_commit(my_type);
}


// maps each station name to a int
unordered_map<string, int> station_name_to_id(const vector<string>& station_names) {
    unordered_map<string, int> out;
    for (int i = 0; i < station_names.size(); i ++) {
        out[station_names[i]] = i;
    }
    return out;
}

// count how many platforms, it is just the total degree of the graph
// degree of each node (or station) = number of outgoing links for that station = number of platforms for that station
int how_many_platforms(const adjacency_matrix& mat) {
    int cnt = 0;
    for (int r = 0; r < mat.size(); r ++) {
        for (int c = 0; c < mat[0].size(); c ++) {
            cnt += (mat[r][c] != 0) ? 1 : 0;
        }
    }
    return cnt;
}


// a platform is identified by src station id and dest station id
// creates a hashmap so that we can identify a platform from the src station id and dest station id
// and also fills the vector<Platforms*> with a new platform with the correct popularity
// also fills the station name of platform in platform_id_to_string
unordered_map<int, unordered_map<int, int>> platforms_to_id(const vector<string>& station_names, 
                                                            const adjacency_matrix& mat, 
                                                            const unordered_map<string, int>& station_ids,
                                                            const vector<size_t>& popularities,
                                                            vector<Platform>& platforms,
                                                            vector<string>& platform_id_to_string) {
    int cnt = 0;
    unordered_map<int, unordered_map<int, int>> out;
    for (int r = 0; r < mat.size(); r ++) {
        for (int c = 0; c < mat[0].size(); c ++) {
            if (mat[r][c] == 0) continue;
            out[r][c] = cnt;

            // set the src_station_id and dest_station_id, impt when saving states
            // then set platform (actually station) popularity
            // then set link distance
            platforms.emplace_back(r, c, popularities[r], mat[r][c]);
            platform_id_to_string.push_back(station_names[r]);
            cnt ++;
        }
    }
    return out;
}

// links the platforms for each line
void link_platforms(char line, const vector<string>& station_line, 
                    unordered_map<int, unordered_map<int, int>>& platform_ids,
                    unordered_map<string, int>& station_ids,
                    vector<Platform>& platforms) {
    
    
    
    for (int i = 0; i < station_line.size() - 1; i ++) {
        if (i == 0 && i + 1 == station_line.size() - 1) {
            int pa_id = platform_ids[station_ids[station_line[0]]][station_ids[station_line[1]]];
            int pb_id = platform_ids[station_ids[station_line[1]]][station_ids[station_line[0]]];
            
            platforms[pa_id].output_platforms[line] = pb_id;
            platforms[pb_id].input_platforms.push_back(pa_id);

            platforms[pb_id].output_platforms[line] = pa_id;
            platforms[pa_id].input_platforms.push_back(pb_id);
        } else if (i == 0) {
            int pa_id = platform_ids[station_ids[station_line[0]]][station_ids[station_line[1]]];
            int pb_id = platform_ids[station_ids[station_line[1]]][station_ids[station_line[2]]];

            platforms[pa_id].output_platforms[line] = pb_id;
            platforms[pb_id].input_platforms.push_back(pa_id);

            int pc_id = platform_ids[station_ids[station_line[1]]][station_ids[station_line[0]]];
            platforms[pc_id].output_platforms[line] = pa_id;
            platforms[pa_id].input_platforms.push_back(pc_id);
        } else if (i + 1 == station_line.size() - 1) {
            int pa_id = platform_ids[station_ids[station_line[i]]][station_ids[station_line[i + 1]]];
            int pb_id = platform_ids[station_ids[station_line[i + 1]]][station_ids[station_line[i]]];

            platforms[pa_id].output_platforms[line] = pb_id;
            platforms[pb_id].input_platforms.push_back(pa_id);

            int pc_id = platform_ids[station_ids[station_line[i]]][station_ids[station_line[i - 1]]];
            platforms[pb_id].output_platforms[line] = pc_id;
            platforms[pc_id].input_platforms.push_back(pb_id);
        } else {
            int pa_id = platform_ids[station_ids[station_line[i]]][station_ids[station_line[i + 1]]];
            int pb_id = platform_ids[station_ids[station_line[i + 1]]][station_ids[station_line[i + 2]]];
            platforms[pa_id].output_platforms[line] = pb_id;
            platforms[pb_id].input_platforms.push_back(pa_id);

            int pc_id = platform_ids[station_ids[station_line[i + 1]]][station_ids[station_line[i]]];
            int pd_id = platform_ids[station_ids[station_line[i]]][station_ids[station_line[i - 1]]];
            platforms[pc_id].output_platforms[line] = pd_id;
            platforms[pd_id].input_platforms.push_back(pc_id);
        } 
    }
}

// for each MPI process to know which platforms it has.
vector<int> assign_platform_ids_to_process(int rank, int total_process, int total_platforms) {
    vector<int> out;
    for (int i = rank; i < total_platforms; i += total_process) {
        out.push_back(i);
    }
    return out;
}

vector<int> map_platform_to_rank(int total_platforms, int total_process) {
    vector<int> out(total_platforms, 0);
    for (int i = 0; i < total_platforms; i ++) out[i] = i % total_process;
    return out;
}

// for each line, get the terminal platform ids
// idx 0 for green line, idx 1 for yellow line, idx 2 for blue line
vector<vector<int>> get_terminal_platform_ids_for_each_line(const unordered_map<char, vector<string>>& station_lines,
                                                            unordered_map<int, unordered_map<int, int>>& platform_ids,
                                                            unordered_map<string, int>& station_ids) {
    vector<vector<int>> out(3, vector<int>());
    char lines[] = "gyb";
    for (int i = 0; i < 3; i ++) {
        char line = lines[i];
        const vector<string>& station_line = station_lines.at(line);
        int start_platform_id = platform_ids[station_ids[station_line[0]]][station_ids[station_line[1]]];

        int len = station_line.size();
        int end_platform_id = platform_ids[station_ids[station_line[len - 1]]][station_ids[station_line[len - 2]]];
        out[i].push_back(start_platform_id);
        out[i].push_back(end_platform_id);
    }
    return out;
}

// each MPI process needs to call this process to spawn the train. Should the MPI process have
// num_trains is changed to a vector, as in simulate, num_trains is a const unordered_map, which means it values 
// cannot be changed
// for terminal_platform_ids_for_each_line, idx 0 is for green, idx 1 is for yello, idx 2 is for blue
// eg. terminal_platform_ids_for_each_line[0][1] means the end (or right) terminal platform for the green line
// if a platform belong to MPI process of rank i, needs to spawn a train, this process will call send_in with Train of
// the correct color and id
void spawn_trains(vector<vector<int>>& terminal_platform_ids_for_each_line, vector<int>& platform_which_process,
                  vector<int>& num_trains, vector<Platform>& platforms, int *count_of_trains_already_spawned, int tick, int rank) {
    char lines[] = "gyb";

    // green, then yellow, then blue
    for (int i = 0; i < 3; i ++) {
        // left terminal then right terminal
        for (int pos = 0; pos <= 1; pos ++) {
            if (num_trains[i] == 0) continue;

            int platform_id = terminal_platform_ids_for_each_line[i][pos];
            
            // if platform belongs to this MPI process
            if (platform_which_process[platform_id] == rank) {
                platforms[platform_id].send_in({lines[i], *count_of_trains_already_spawned}, tick);
            }
            (*count_of_trains_already_spawned) ++;
            num_trains[i] --;
        }
    }                
}


void sendout_sendin_trains(int tick,
                           vector<int>& my_platform_ids, 
                           vector<int>& platform_which_process, 
                           vector<Platform>& platforms, MPI_Datatype mpi_train) {
    vector<MPI_Request> mpi_requests;
    

    // this for loop does MPI_Isend for all platforms assign to this rank
    for (int id : my_platform_ids) {
        Platform& platform = platforms[id];
        Pair p = platform.send_out(tick);
        // send out trains, must send to each train in output_platforms, even if no trains to send
        for (const auto& [line, dest_platform_id] : platform.output_platforms) {
            MPI_Request request;
            
            // even if we send to the same process, it does not matter
            // i have tested using Isend, Irecv in the same process, and it works
            // invariant: must send to all output platforms, must recv from all input platforms
            
            // IMPT: the tag must have the id of the the sending platform id, to differentiate between the messages
            if (!(p.train == INVALID_TRAIN) && p.train.line == line) {
                // send the train
                MPI_Isend(&(p.train), 1, mpi_train, platform_which_process[dest_platform_id], id, MPI_COMM_WORLD, &request);
            } else {
                // send invalid train
                MPI_Isend(&INVALID_TRAIN, 1, mpi_train, platform_which_process[dest_platform_id], id, MPI_COMM_WORLD, &request);
            }
            
            mpi_requests.push_back(request);
        }
    }

    vector<vector<Train>> trains_received(my_platform_ids.size(), vector<Train>());
    
    // for each platform, receive trains from input_platforms
    for (int i = 0; i < my_platform_ids.size(); i ++) {
        int id = my_platform_ids[i];
        Platform& platform = platforms[id];
        
        
        vector<Train>& recv_buffer = trains_received[i];
        recv_buffer.resize(platform.input_platforms.size());

        for (int j = 0; j < platform.input_platforms.size(); j ++) {
            
            MPI_Request request;
            int input_platform_id = platform.input_platforms[j];

            // IMPT: tag must be the input_platform id
            MPI_Irecv(&(recv_buffer[j]), 1, mpi_train, platform_which_process[input_platform_id], input_platform_id, MPI_COMM_WORLD, &request);

            mpi_requests.push_back(request);
        }
    }

   
    // now wait all
    MPI_Waitall(mpi_requests.size(), mpi_requests.data(), MPI_STATUS_IGNORE);

    for (int i = 0; i < my_platform_ids.size(); i ++) {
        int id = my_platform_ids[i];
        platforms[id].send_in(trains_received[i], tick);
    }
}

void push_train_in_for_my_platforms(int tick, vector<int>& my_platform_ids, vector<Platform>& platforms) {
    for (int id : my_platform_ids) {
        platforms[id].push_train_to_platform(tick);
    }
}

void save_platform_states(int tick, vector<int>& my_platform_ids, vector<Platform>& platforms) {
    for (int id : my_platform_ids) {
        platforms[id].save_all_states(tick);
    }
}


void print(vector<int> arr) {
    for (int x : arr) std::cout << x << " ";
}

vector<State> collect_all_states(vector<int>& my_platform_ids, vector<Platform>& platforms) {
    vector<State> out;
    for (int id : my_platform_ids) {
        out.insert(out.end(), platforms[id].saved_states.begin(), platforms[id].saved_states.end());
    }
    return out;
}

/*int* get_num_states_per_process(int* num_states, int rank, int total_processes) {
    int* num_states_per_process = nullptr;

    if (rank == 0) num_states_per_process = new int[total_processes];

    MPI_Gather(num_states, 1, MPI_INT, num_states_per_process, 1, MPI_INT, 0, MPI_COMM_WORLD);
    return num_states_per_process;
}

int get_total_num_of_states(int* num_states_per_process, int total_process) {
    int sum = 0;
    for (int i = 0; i < total_process; i ++) sum += num_states_per_process[i];
    return sum;
}

State* get_all_states(State* my_states, 
                      int* num_states_per_process, // only valid at rank 0
                      int my_state_size,
                      int rank, 
                      int total_processes, 
                      MPI_Datatype mpi_state) {
    int* displacement = nullptr;
    State* all_states = nullptr;

    if (rank == 0) {
        displacement = new int[total_processes];
        displacement[0] = 0;
        for (int i = 1; i < total_processes; i ++) displacement[i] = displacement[i - 1] + num_states_per_process[i - 1];

        all_states = new State[displacement[total_processes - 1]];
    }

    
    MPI_Gatherv(my_states, 
                my_state_size, 
                mpi_state, 
                all_states, 
                num_states_per_process, 
                displacement, 
                mpi_state, 
                0, MPI_COMM_WORLD);
    
    return all_states;
}*/


void simulate(size_t num_stations, const vector<string> &station_names, const std::vector<size_t> &popularities,
              const adjacency_matrix &mat, const unordered_map<char, vector<string>> &station_lines, size_t ticks,
              const unordered_map<char, size_t> num_trains, size_t num_ticks_to_print, size_t mpi_rank,
              size_t total_processes) {
    
    unordered_map<string, int> station_ids = station_name_to_id(station_names);
    
    vector<Platform> platforms;
    vector<string> platform_ids_to_string;
    unordered_map<int, unordered_map<int, int>> platform_ids = platforms_to_id(station_names, mat, station_ids, popularities, platforms, 
                                                                               platform_ids_to_string);

    for (const auto& [color, line] : station_lines) {
        link_platforms(color, line, platform_ids, station_ids, platforms);
    } 

    vector<int> platform_which_process = map_platform_to_rank(platforms.size(), (int) total_processes);
    
    vector<int> my_platform_ids = assign_platform_ids_to_process(mpi_rank, total_processes, platforms.size());
    vector<vector<int>> terminal_platform_ids_for_each_line = get_terminal_platform_ids_for_each_line(station_lines, platform_ids, station_ids);
    
    vector<int> num_trains_per_line = {(int) num_trains.at('g'), (int) num_trains.at('y'), (int) num_trains.at('b')};
    int count_of_trains_spawned = 0;

    MPI_Datatype mpi_train, mpi_state;
    create_mpi_Train(&mpi_train);
    create_mpi_State(&mpi_state);

    for (int tick = 0; tick < ticks; tick++) {
        spawn_trains(terminal_platform_ids_for_each_line, platform_which_process, 
                 num_trains_per_line, platforms, &count_of_trains_spawned, tick, mpi_rank);

        sendout_sendin_trains(tick, my_platform_ids, platform_which_process, platforms, mpi_train);

        push_train_in_for_my_platforms(tick, my_platform_ids, platforms);

        if (tick >= ticks - num_ticks_to_print) save_platform_states(tick, my_platform_ids, platforms);
        
    }

    
    // rank 0 to gather all states
    vector<State> my_states = collect_all_states(my_platform_ids, platforms);
    int my_state_size = my_states.size();

    // gather num of states
    int* num_states_per_process = new int[total_processes];
    MPI_Gather(&my_state_size, 1, MPI_INT, num_states_per_process, 1, MPI_INT, 0, MPI_COMM_WORLD);

    
    
    // get total num of states
    int total_states = 0;
    if (mpi_rank == 0) {
        for (int i = 0; i < total_processes; i ++) total_states += num_states_per_process[i];
    }
    

    // calculate displacement
    int* displacements = new int[total_processes];
    displacements[0] = 0;
    if (mpi_rank == 0) {
        for (int i = 1; i < total_processes; i ++) {
            displacements[i] = displacements[i - 1] + num_states_per_process[i - 1];
        }
    }
    
    State* states = new State[total_states];
    MPI_Gatherv(my_states.data(), my_state_size, mpi_state, states, num_states_per_process, displacements, mpi_state, 0, MPI_COMM_WORLD);
    
    if (mpi_rank == 0) {
        print_all_states_ptr(states, total_states, num_ticks_to_print, ticks, station_names);
    }
    


    
    

    
}