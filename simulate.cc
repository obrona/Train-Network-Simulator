#include <vector>
#include <string>
#include <unordered_map>

#include "structs.hpp"

using std::string;
using std::unordered_map;
using std::vector;
using adjacency_matrix = std::vector<std::vector<size_t>>;


// create mpi_Train type
/*void create_mpi_Train(MPI_Datatype *my_type) {
    int num_elements = 2;  // Number of elements in the struct
    int block_lengths[] = {1, 1};  // Number of items of each data type in the struct
    MPI_Datatype types[] = {MPI_CHAR, MPI_INT};  // Data types of each struct element
    MPI_Aint offsets[] = {offsetof(Train, line), offsetof(Train, id)};  // Offsets of each struct element
    MPI_Type_create_struct(num_elements, block_lengths, offsets, types, my_type);
    MPI_Type_commit(my_type);
}*/


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

// create all platforms
vector<Platform*> create_platforms(int total_platforms) {
    return vector(total_platforms, new Platform);
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
            platforms.push_back(new Platform(popularites[r]));
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

// for each MPI process to know which platforms it has
vector<int> assign_platform_ids_to_process(int rank, int total_process, int total_platforms) {
    vector<int> out;
    for (int platform = rank; platform < total_platforms; platform += total_process) {
        out.push_back(platform);
    }
    return out;
}



void simulate(size_t num_stations, const vector<string> &station_names, const std::vector<size_t> &popularities,
              const adjacency_matrix &mat, const unordered_map<char, vector<string>> &station_lines, size_t ticks,
              const unordered_map<char, size_t> num_trains, size_t num_ticks_to_print, size_t mpi_rank,
              size_t total_processes) {
    
    unordered_map<string, int> station_ids = station_name_to_id(station_names);
    
    vector<Platform*> platforms;
    unordered_map<int, unordered_map<int, int>> platform_ids = platforms_to_id(station_names, mat, station_ids, popularities, platforms);       
    
}