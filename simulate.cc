#include <vector>
#include <string>
#include <unordered_map>
#include <mpi.h>
#include "structs.hpp"

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
    int block_lengths[] = {1, 4};  // Number of items of each data type in the struct
    MPI_Datatype types[] = {MPI_CHAR, MPI_INT};  // Data types of each struct element
    MPI_Aint offsets[] = {offsetof(State, line), offsetof(State, id)};  // Offsets of each struct element
    MPI_Type_create_struct(num_elements, block_lengths, offsets, types, my_type);
    MPI_Type_commit(my_type);
}


// maps each station name to a int
unordered_map<string, int> station_name_to_id(const vector<string> &station_names) {
    unordered_map<string, int> out;
    for (int i = 0; i < station_names.size(); i ++) {
        out[station_names[i]] = i;
    }
    return out;
}

// count how many platforms, it is just the total degree of the graph
// degree of each node (or station) = number of outgoing links for that station = number of platforms for that station
int how_many_platforms(const adjacency_matrix &mat) {
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
unordered_map<int, unordered_map<int, int>> platforms_to_id(const vector<string> &station_names, 
                                                            const adjacency_matrix &mat, 
                                                            const unordered_map<string, int> &station_ids,
                                                            const vector<size_t> &popularites,
                                                            vector<Platform*> platforms) {
    int cnt = 0;
    unordered_map<int, unordered_map<int, int>> out;
    for (int r = 0; r < mat.size(); r ++) {
        for (int c = 0; c < mat[0].size(); c ++) {
            if (mat[r][c] == 0) continue;
            out[r][c] = cnt;

            // set the platform popularity
            // the station in which the platform is on is the src station, so is r
            // and also the link distance
            platforms.push_back(new Platform(popularites[r], mat[r][c]));
            cnt ++;
        }
    }
    return out;
}

// links the platforms for each line
void link_platforms(char line, const vector<string> &station_line, 
                    unordered_map<int, unordered_map<int, int>> platform_ids,
                    unordered_map<string, int> &station_ids,
                    vector<Platform*> platforms) {
    
    
    
    for (int i = 0; i < station_line.size() - 1; i ++) {
        if (i == 0 && i + 1 == station_line.size() - 1) {
            int pa_id = platform_ids[station_ids[station_line[0]]][station_ids[station_line[1]]];
            int pb_id = platform_ids[station_ids[station_line[1]]][station_ids[station_line[0]]];
            
            platforms[pa_id]->output_platforms[line] = pb_id;
            platforms[pb_id]->input_platforms.push_back(pa_id);

            platforms[pb_id]->output_platforms[line] = pa_id;
            platforms[pa_id]->input_platforms.push_back(pb_id);
        } else if (i == 0) {
            int pa_id = platform_ids[station_ids[station_line[0]]][station_ids[station_line[1]]];
            int pb_id = platform_ids[station_ids[station_line[1]]][station_ids[station_line[2]]];

            platforms[pa_id]->output_platforms[line] = pb_id;
            platforms[pb_id]->input_platforms.push_back(pa_id);

            int pc_id = platform_ids[station_ids[station_line[1]]][station_ids[station_line[0]]];
            platforms[pc_id]->output_platforms[line] = pa_id;
            platforms[pa_id]->input_platforms.push_back(pc_id);
        } else if (i + 1 == station_line.size() - 1) {
            int pa_id = platform_ids[station_ids[station_line[i]]][station_ids[station_line[i + 1]]];
            int pb_id = platform_ids[station_ids[station_line[i + 1]]][station_ids[station_line[i]]];

            platforms[pa_id]->output_platforms[line] = pb_id;
            platforms[pb_id]->input_platforms.push_back(pa_id);

            int pc_id = platform_ids[station_ids[station_line[i]]][station_ids[station_line[i - 1]]];
            platforms[pb_id]->output_platforms[line] = pc_id;
            platforms[pc_id]->input_platforms.push_back(pb_id);
        } else {
            int pa_id = platform_ids[station_ids[station_line[i]]][station_ids[station_line[i + 1]]];
            int pb_id = platform_ids[station_ids[station_line[i + 1]]][station_ids[station_line[i + 2]]];
            platforms[pa_id]->output_platforms[line] = pb_id;
            platforms[pb_id]->input_platforms.push_back(pa_id);

            int pc_id = platform_ids[station_ids[station_line[i + 1]]][station_ids[station_line[i]]];
            int pd_id = platform_ids[station_ids[station_line[i]]][station_ids[station_line[i - 1]]];
            platforms[pc_id]->output_platforms[line] = pd_id;
            platforms[pd_id]->input_platforms.push_back(pc_id);
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
                                                            unordered_map<string, int> &station_ids) {
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
                  vector<int>& num_trains, vector<Platform*> platforms, int* count_of_trains_already_spawned, int tick, int rank) {
    char lines[] = "gyb";

    // green, then yellow, then blue
    for (int i = 0; i < 3; i ++) {
        // left terminal then right terminal
        for (int pos = 0; pos <= 1; pos ++) {
            if (num_trains[i] == 0) continue;

            int platform_id = terminal_platform_ids_for_each_line[i][pos];
            
            // if platform belongs to this MPI process
            if (platform_which_process[platform_id] == rank) {
                platforms[platform_id]->send_in({lines[i], *count_of_trains_already_spawned}, tick);
            }
            (*count_of_trains_already_spawned) ++;
            num_trains[i] --;
        }
    }                
}                  



void simulate(size_t num_stations, const vector<string> &station_names, const std::vector<size_t> &popularities,
              const adjacency_matrix &mat, const unordered_map<char, vector<string>> &station_lines, size_t ticks,
              const unordered_map<char, size_t> num_trains, size_t num_ticks_to_print, size_t mpi_rank,
              size_t total_processes) {
    
    unordered_map<string, int> station_ids = station_name_to_id(station_names);
    
    vector<Platform*> platforms;
    unordered_map<int, unordered_map<int, int>> platform_ids = platforms_to_id(station_names, mat, station_ids, popularities, platforms);

    for (const auto& [color, line] : station_lines) {
        link_platforms(color, line, platform_ids, station_ids, platforms);
    } 

    vector<int> platform_which_process = map_platform_to_rank(platforms.size(), (int) total_processes);
    vector<vector<int>> terminal_platform_ids_for_each_line = get_terminal_platform_ids_for_each_line(station_lines, platform_ids, station_ids);

    vector<int> num_trains_per_line = {(int) num_trains.at('g'), (int) num_trains.at('y'), (int) num_trains.at('b')};
    
}

int main() {
    return 0;
}