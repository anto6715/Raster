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
#include "raster.h"


using namespace std;
using namespace std::chrono;


int main() {
    int row, column;
    // parameters of algorithm
    double precision = -4.5;
    int threshold = 2;
    int min_size = 3;
    string name_file = "/home/antonio/Scrivania/Datasets/S-sets/s1.csv";

    // dataset dimension
    getDim(name_file, row, column);
    cout << row << endl;

    double* dataset_storage = new double[row*column* sizeof(double)];
    double** dataset = new double*[row * sizeof(double)];
    for (int i = 0; i < row; i++) {
        dataset[i] = &dataset_storage[i*column];
    }

    // data structure declaration
    hashmap projection;
    vectorSet2 clusters;
    hashmapUnset all_points;

    // read data from csv file
    loadData(dataset, name_file, column);
    mapToTilesPrime(dataset, precision, threshold, row, projection, all_points);
    clusteringTiles(projection, min_size, clusters);
    printAllPointsClustered(clusters, all_points, "sequential");


    return 0;
}

