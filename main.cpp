#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cmath>
#include <map>
#include <cstring>
#include <boost/functional/hash.hpp>
#include <unordered_map>
#include <chrono>
#include <unordered_set>

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
void clusteringTiles(unordered_map<array<int, 2>, int, container_hasher> &tiles, int min_size, vector<unordered_set<array<int, 2>, container_hasher>> &cluster);
void printClusters(vector<unordered_set<array<int, 2>, container_hasher>> &cluster, double precision);


int main() {
    int row, column;
    double precision = 5;
    int threshold = 3;
    int min_size = 5;
    string name_file = "/home/antonio/Scrivania/Datasets/data_10_shuffled2.csv";
    column = 2;
    row = countRow(name_file);

    /*
    double ** dataset = new double*[row];
    for (int i = 0; i < row; i++) {
        dataset[i] = new double[column];
    }
    */
    double* dataset_storage = new double[row*column* sizeof(double)];
    double** dataset = new double*[row * sizeof(double)];
    for (int i = 0; i < row; i++) {
        dataset[i] = &dataset_storage[i*column];
    }
    // unordered version
    unordered_map<array<int, 2>, int, container_hasher> projection;
    vector<unordered_set<array<int, 2>, container_hasher>> cluster;

    // read data from csv file
    loadData(dataset, name_file);

    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    // points agglomeration
    mapToTiles(dataset, precision, threshold, row, projection);
    cout << "projection: " << projection.size() << endl;
    // creating clusters
    clusteringTiles(projection, min_size, cluster);

    high_resolution_clock::time_point t2 = high_resolution_clock::now();

    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "It took me " << time_span.count() << " seconds." << endl;
    printClusters(cluster, precision);

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

void mapToTiles(double **m, double precision, int threshold, int n, unordered_map<array<int, 2>, int, container_hasher> &projection) {
    double scalar;
    unordered_map<array<int, 2>, int, container_hasher>::iterator it;
    scalar = pow(10, precision);
    for (int i = 0; i < n; i++) {
        int lat =(int) (m[i][0] * scalar);
        int lon =(int) (m[i][1] * scalar);
        array<int, 2> tile;
        tile = {lat,lon};
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

void getNeighbors(array<int, 2> coordinate, unordered_map<array<int, 2>, int, container_hasher> &tiles, unordered_set<array<int, 2>, container_hasher> &result) {
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
        it = tiles.find(neighbors[i]);
        if (it != tiles.end()) {
            result.insert(it -> first);
            tiles.erase(it++);
        }
    }
} //unordered

void clusteringTiles(unordered_map<array<int, 2>, int, container_hasher> &tiles, int min_size, vector<unordered_set<array<int, 2>, container_hasher>> &cluster) {
    unordered_map<array<int, 2>, int, container_hasher>::iterator iterator;
    unordered_set<array<int, 2>, container_hasher>::iterator iterator_set;
    // read all tiles
    while ((iterator = tiles.begin()) != tiles.end()) {
        // read and remove first element recursively
        array<int, 2> x = iterator->first;
        tiles.erase(iterator++);

        unordered_set<array<int, 2>, container_hasher> visited;
        visited.insert(x);

        // get neighboor of tile in exam
        unordered_set<array<int, 2>, container_hasher> to_check;
        getNeighbors(x,tiles, to_check);

        // for each neighbor, try to find respectively neighbor in order to add they to single cluster
        while (!(to_check.size() == 0)) {
            array<int, 2> value = *to_check.begin() ;
            to_check.erase((to_check.begin()));
            visited.insert(value);

            unordered_set<array<int, 2>, container_hasher> temp;

            getNeighbors(value,tiles, temp);
            while (!(temp.size() == 0)) {
                to_check.insert(*temp.begin());
                temp.erase((temp.begin()));
            }
        }
        // validate visited as cluster if is size is >= min size
        if (visited.size() >= min_size) {
            cluster.push_back(visited);
        }

    }
} // unordered

void printClusters(vector<unordered_set<array<int, 2>, container_hasher>> &cluster, double precision) {
    cout.precision(10);
    cout <<  "n° cluster: " << cluster.size() << endl;
    unordered_set<array<int, 2>, container_hasher>::iterator it;
    double scalar = pow(10, precision);
    int count = 0;
    for (int j = 0; j < cluster.size(); j++) {
        cout << "Cluster n° " << j << " with size " << cluster.at(j).size() << ": " << endl;
        it = cluster.at(j).begin();
        for (int i = 0; i < cluster.at(j).size(); i++) {
            count++;
            cout << (*it)[0]/scalar << " ";
            cout << (*it)[1]/scalar << endl;
            it++;
        }
    }
    cout << "elementi: " << count << endl;
} // semi-unordered

void printMatrix(double **matrix, int n, int m) {
    cout.precision(17);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            cout << matrix[i][j] << " ";
        }
        cout << "\n";
    }
}