#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <map>
#include <cstring>
#include <boost/functional/hash.hpp>
#include <unordered_map>
#include <chrono>
#include <unordered_set>

#define TYPE "DOUBLE" // set as parameters?
using namespace std;
using namespace std::chrono;

void hash_combine(std::size_t& seed, std::size_t value) {
    seed ^= value + 0x9e3779b9 + (seed<<6) + (seed>>2);
}
struct container_hasher {
    template<class T>
    std::size_t operator()(const T& c) const {
        std::size_t seed = 0;
        for(const auto& elem : c) {
            hash_combine(seed, std::hash<typename T::value_type>()(elem));
        }
        return seed;
    }
};

//void printMatrix(double **matrix, int n, int m);
void loadData(double **m, string name_file);
int countRow(string name_file);

// unordered
void getNeighbors(array<int, 2> coordinate, unordered_map<array<int, 2>, int, container_hasher> &tiles, unordered_set<array<int, 2>, container_hasher> &result);
void mapToTiles(double **m, double precision, int threshold, int n, unordered_map<array<int, 2>, int, container_hasher> &projection);
void mapToTilesPrime(double **m, double precision, int threshold, int n, unordered_map<array<int, 2>, int, container_hasher> &projection, unordered_map<array<int, 2>, unordered_set<array<double , 2>, container_hasher>, container_hasher> &all_points); //raster'
void clusteringTiles(unordered_map<array<int, 2>, int, container_hasher> &projection, int min_size, vector<unordered_set<array<int, 2>, container_hasher>> &clusters);
void printClusters(vector<unordered_set<array<int, 2>, container_hasher>> &clusters, double precision);
void printAllPointsClustered(vector<unordered_set<array<int, 2>, container_hasher>> &clusters, unordered_map<array<int, 2>, unordered_set<array<double , 2>, container_hasher>, container_hasher> &all_points); // raster'

int main() {
    int row, column;

    // parameters of algorithm
    double precision = 4.2;
    int threshold = 2;
    int min_size = 3;
    string name_file = "../s1.csv";//"/home/antonio/Scrivania/Datasets/data_10_shuffled2.csv";

    // dataset dimension
    column = 2;
    row = countRow(name_file);


    double* dataset_storage = new double[row*column* sizeof(double)];
    double** dataset = new double*[row * sizeof(double)];
    for (int i = 0; i < row; i++) {
        dataset[i] = &dataset_storage[i*column];
    }

    // data structure to contain data
    unordered_map<array<int, 2>, int, container_hasher> projection;
    vector<unordered_set<array<int, 2>, container_hasher>> clusters;
    unordered_map<array<int, 2>, unordered_set<array<double , 2>, container_hasher>, container_hasher> all_points;

    // read data from csv file
    loadData(dataset, name_file);

    // get start time for benchmark
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    // points agglomeration
    //mapToTiles(dataset, precision, threshold, row, projection);
    mapToTilesPrime(dataset, precision, threshold, row, projection, all_points);
    cout << "projection after threshold: " << projection.size() << endl;

    // creating clusters
    clusteringTiles(projection, min_size, clusters);

    // get end time for benchmark
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

    //printClusters(cluster, precision);
    printAllPointsClustered(clusters, all_points);

    std::cout << "\n" << "It took me " << time_span.count() << " seconds." << endl;
    return 0;
}

int countRow(string name_file) {
    ifstream inputFile(name_file);
    int row = 0;
    while (inputFile) {
        string s;
        if (!getline(inputFile, s)) break;
        row++;
    }
    if (!inputFile.eof()) {
        cerr << "Could not read file " << "0_data_generators_data_1_shuffled" << "\n";
        __throw_invalid_argument("File not found.");
    }
    return row;
}

void loadData(double** m, string name_file) {
    ifstream inputFile(name_file);
    int row = 0;
    while (inputFile) {

        string s;
        if (!getline(inputFile, s)) break;
        if (s[0] != '#') {
            istringstream ss(s);
            while (ss) {
                for (int i = 0; i < 2; i++) {
                    string line;
                    if (!getline(ss, line, ','))
                        break;
                    try {
                        m[row][i] = stod(line);
                    }
                    catch (const std::invalid_argument e) {
                        cout << "NaN found in file " << "../0_data_generators_data_1_shuffled.csv" << " line " << row
                             << endl;
                        e.what();
                    }
                }
            }


        }
        row++;
    }
    if (!inputFile.eof()) {
        cerr << "Could not read file " << "0_data_generators_data_1_shuffled" << "\n";
        __throw_invalid_argument("File not found.");
    }
}

// raster'
void mapToTilesPrime(double **m, double precision, int threshold, int n, unordered_map<array<int, 2>, int, container_hasher> &projection, unordered_map<array<int, 2>, unordered_set<array<double , 2>, container_hasher>, container_hasher> &all_points) {
    double scalar;
    int count= 0;
    unordered_map<array<int, 2>, int, container_hasher>::iterator it;
    unordered_map<array<int, 2>, unordered_set<array<double , 2>, container_hasher>, container_hasher>::iterator it_map_all_points;
    unordered_set<array<double , 2>, container_hasher>::iterator it_set_all_points;
    scalar = pow(10, precision);
    int lat, lon;
    for (int i = 0; i < n; i++) {
        if (TYPE == "DOUBLE"){
            lat =(int) (m[i][0] * scalar);
            lon =(int) (m[i][1] * scalar);
        } else if (TYPE == "INT"){
            lat =(int) (m[i][0] / scalar);
            lon =(int) (m[i][1] / scalar);
        }
        array<int, 2> tile = {lat,lon};
        it = projection.find(tile);
        if (it != projection.end()) {
            it->second++;
        } else {
            projection[tile] = 1;
        }

        it_map_all_points = all_points.find(tile);
        if (it_map_all_points != all_points.end()) {
            (it_map_all_points -> second).insert({m[i][0],m[i][1]});
        } else {
            unordered_set<array<double , 2>, container_hasher> point;
            point.insert({m[i][0], m[i][1]});
            all_points[tile] = point;
        }


    }
    cout << "projection before threshold: " << projection.size() << endl;

    // remove tile with count < threshold
    it = projection.begin();
    while (it != projection.end()) {
        if (it -> second < threshold) {
            projection.erase(it++);
        } else {
            it++;
        }
    }
}

void mapToTiles(double **m, double precision, int threshold, int n, unordered_map<array<int, 2>, int, container_hasher> &projection) {
    double scalar;
    unordered_map<array<int, 2>, int, container_hasher>::iterator it;
    scalar = pow(10, precision);
    int lat, lon;
    for (int i = 0; i < n; i++) {
        if (TYPE == "DOUBLE"){
            lat =(int) (m[i][0] * scalar);
            lon =(int) (m[i][1] * scalar);
        } else if (TYPE == "INT"){
            lat =(int) (m[i][0] / scalar);
            lon =(int) (m[i][1] / scalar);
        }
        array<int, 2> tile = {lat,lon};
        it = projection.find(tile);
        if (it != projection.end()) {
            it->second++;
        } else {
            projection[tile] = 1;
        }
    }

    // remove tile with count < threshold
    it = projection.begin();
    while (it != projection.end()) {
        if (it -> second < threshold) {
            projection.erase(it++);
        } else {
            it++;
        }
    }
} //unordered

void getNeighbors(array<int, 2> coordinate, unordered_map<array<int, 2>, int, container_hasher> &projection, unordered_set<array<int, 2>, container_hasher> &result) {
    int x = coordinate[0];
    int y = coordinate[1];
    unordered_map<array<int, 2>, int, container_hasher>::iterator it;
    // neighboring generation of coordinate
    array<array<int, 2>,8> neighbors;
    neighbors[0] = {x+1, y};
    neighbors[1] = {x-1, y};
    neighbors[2]  = {x, y+1};
    neighbors[3]  = {x, y-1};
    neighbors[4]  = {x+1, y-1};
    neighbors[5]  = {x+1, y+1};
    neighbors[6]  = {x-1, y-1};
    neighbors[7]  = {x-1, y+1};
    // if a neighbor is present in tiles, add it in result
    for (int i = 0; i < 8; i++) {
        it = projection.find(neighbors[i]);
        if (it != projection.end()) {
            result.insert(it -> first);
            projection.erase(it++);
        }
    }
} //unordered

void clusteringTiles(unordered_map<array<int, 2>, int, container_hasher> &projection, int min_size, vector<unordered_set<array<int, 2>, container_hasher>> &clusters) {
    unordered_map<array<int, 2>, int, container_hasher>::iterator iterator;

    // read all tiles
    while ((iterator = projection.begin()) != projection.end()) {
        // read and remove first element recursively
        array<int, 2> x = iterator->first;
        projection.erase(iterator++);

        unordered_set<array<int, 2>, container_hasher> visited;
        visited.insert(x);

        // get neighboors of tile in exam
        unordered_set<array<int, 2>, container_hasher> to_check;
        getNeighbors(x,projection, to_check);

        // for each neighbor, try to find respectively neighbor in order to add they to single cluster
        while (!to_check.empty()) {
            array<int, 2> value = *to_check.begin() ;
            to_check.erase((to_check.begin()));
            visited.insert(value);

            unordered_set<array<int, 2>, container_hasher> temp;

            getNeighbors(value,projection, temp);
            while (!temp.empty()) {
                to_check.insert(*temp.begin());
                temp.erase((temp.begin()));
            }
        }
        // validate visited as cluster if is size is >= min size
        if (visited.size() >= min_size) {
            clusters.push_back(visited);
        }

    }
} // unordered

void printClusters(vector<unordered_set<array<int, 2>, container_hasher>> &clusters, double precision) {
    cout.precision(10);
    cout <<  "n° cluster: " << clusters.size() << endl;
    unordered_set<array<int, 2>, container_hasher>::iterator it;

    double scalar = pow(10, precision); // to obtain the correct value of tiles

    int count_tiles = 0;
    for (int j = 0; j < clusters.size(); j++) {
        cout << "Cluster n° " << j << " with size " << clusters.at(j).size() << ": " << endl;
        it = clusters.at(j).begin(); // pointer to start of j-th cluster (cluster = list of tiles)
        for (int i = 0; i < clusters.at(j).size(); i++) {
            count_tiles++; // count the total number of tiles clustered
            if (TYPE == "DOUBLE"){
                cout << (*it)[0] / scalar << ",";
                cout << (*it)[1] / scalar << ",";
            } else if (TYPE == "INT"){
                cout << (*it)[0] * scalar << ",";
                cout << (*it)[1] * scalar << ",";
            }

            cout << j << endl;
            it++; // next tile of the actual cluster
        }
    }
    cout << "Tiles clustered: " << count_tiles << endl;
} // semi-unordered

// used only in raster' version
void printAllPointsClustered(vector<unordered_set<array<int, 2>, container_hasher>> &clusters, unordered_map<array<int, 2>, unordered_set<array<double , 2>, container_hasher>, container_hasher> &all_points){
    cout.precision(15);
    ofstream outfile("clustered.csv");
    int temp = 0;
    int count_tiles = 0;
    int count_points = 0;
    unordered_map<array<int, 2>, unordered_set<array<double , 2>, container_hasher>, container_hasher>::iterator it_map_all_points;
    unordered_set<array<double , 2>, container_hasher>::iterator it_set_all_points;
    unordered_set<array<int, 2>, container_hasher>::iterator it;
    for (int j = 0; j < clusters.size(); j++) {
        //cout << "Cluster n° " << j << " with size " << cluster.at(j).size() << ": " << endl;
        it = clusters.at(j).begin(); // pointer to start of j-th cluster in clusters (cluster = list of tiles, clusters = list of cluster)
        for (int i = 0; i < clusters.at(j).size(); i++) {
            it_map_all_points = all_points.find((*it)); // try to find in all_points the tile (with its list of points) from cluster
            if (it_map_all_points != all_points.end()) {
                it_set_all_points = (it_map_all_points -> second).begin(); // pointer to the first element in the list of points associated to the founded tile
                for (int i = 0; i < (it_map_all_points -> second).size(); i++) {
                    outfile << (*it_set_all_points)[0] << ",";
                    outfile << (*it_set_all_points)[1] << ",";
                    outfile << j + 1 <<endl;
                    //cout << (*it_set_all_points)[0] << ",";
                    //cout << (*it_set_all_points)[1] << ",";
                    //cout << j <<endl;
                    it_set_all_points++;
                    count_points++;
                }
                all_points.erase(it_map_all_points++);
            }
            it++;
            count_tiles++;
        }
    }
    it_map_all_points = all_points.begin(); // first tile remaining in all_points
    for (int i = 0; i < all_points.size(); i++) {
        if (it_map_all_points != all_points.end()) {
            it_set_all_points = (it_map_all_points -> second).begin(); // pointer to the first element in the list of points associated to the founded tile
            for (int i = 0; i < (it_map_all_points -> second).size(); i++) {
                outfile << (*it_set_all_points)[0] << ",";
                outfile << (*it_set_all_points)[1] << ",";
                outfile << 0 <<endl;
                it_set_all_points++;
                //count_points++;
                temp++;
            }
            //all_points.erase(it_map_all_points++);
            it_map_all_points++;
        }
    }
    outfile.close();
    cout << "temp: " << temp << endl;
    cout << "Tile not clustered: " << all_points.size() << endl;
    cout << "Tiles clustered: " << count_tiles << endl;
    cout << "Clusters: " << clusters.size() << endl;
    cout << "Points clustered: " << count_points << endl;
    cout << "Points anallized: " << count_points + temp << endl;
}

void printMatrix(double **matrix, int n, int m) {
    cout.precision(17);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            cout << matrix[i][j] << " ";
        }
        cout << "\n";
    }
}