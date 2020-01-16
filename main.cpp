#include <iostream>
#include <vector>
#include <map>
#include "raster.h"
#include "error.h"
#include "boost/program_options.hpp"

using namespace std;
namespace po = boost::program_options;


/*** Default Parameters ***/
const double        DEFAULT_PRECISION = -4.2;
const int           DEFAULT_THRESHOLD = 2;
const int           DEFAULT_MIN_SIZE = 3;
const string        DEFAULT_NAME_FILE = "/home/antonio/Scrivania/Datasets/S-sets/s1.csv";
const string        DEFAULT_OUTPUTFILENAME =  "sequential";


/*!< @struct Params - A structure containing parameters read from the command-line.  */
struct Params {
    string      outputFilename;    /*!< The pathname of the output file */
    double      precision;         /*!< Raster parameter that determines the tile dimension */
    int         threshold;         /*!< Raster parameter that determines if a tile should be retained or not */
    int         min_size;          /*!< Raster parameter that determines the minimum number of tiles required to form a cluster */
    string      name_file;         /*!< The path for the input CSV file */
};


/**
 * This function usse the Boost library in order to handle
 * the arguments passed on the command line
 *
 * @param [in] argc - Count of command line arguments
 * @param [in] argv - Contains command line arguments
 * @param [in,out] params - Structure with default params
 * @return 0 in case of success, -99 n case of arguments error
 */
int handleArgument(int argc, char **argv, Params &params);


/**
 * This function initializes params with default values
 *
 * @param [in,out] params - Empty struct to be initialized
 * @return 0 if success
 */
int initParams(Params &params);



int main(int argc, char **argv) {
    int row, column, returnValue;

    // data structure declaration
    Params          params;
    hashmap         projection;
    vectorSet2      clusters;
    hashmapUnset    all_points;


    initParams(params);
    returnValue = handleArgument(argc, argv, params);
    if (returnValue) {
        cerr << "Can't handle argument" << endl;
        return returnValue;
    }

    // dataset dimension
    returnValue = getDim(params.name_file, row, column);
    if (returnValue) {
        cerr << "Can't read dim of input file" << endl;
        return returnValue;
    }
    cout << row << endl;

    double* dataset_storage = nullptr;
    double** dataset = nullptr;

    dataset_storage = new (nothrow) double[row*column* sizeof(double)];
    if (!dataset_storage)
        return memoryError(__FUNCTION__);

    dataset = new (nothrow) double*[row * sizeof(double)];
    if(!dataset) {
        returnValue =  memoryError(__FUNCTION__);
        goto ON_EXIT;
    }

    for (int i = 0; i < row; i++) {
        dataset[i] = &dataset_storage[i*column];
    }



    // read data from csv file
    returnValue = loadData(dataset, params.name_file, column);
    if(returnValue)
        goto ON_EXIT;


    returnValue = mapToTilesPrime(dataset, params.precision, params.threshold, row, projection, all_points);
    if(returnValue)
        goto ON_EXIT;

    returnValue = clusteringTiles(projection, params.threshold, clusters);
    if(returnValue)
        goto ON_EXIT;

    returnValue = printAllPointsClustered(clusters, all_points, params.outputFilename);
    if(returnValue)
        goto ON_EXIT;

    returnValue = 0;

    ON_EXIT:

    if (dataset != nullptr)
        delete[] dataset, dataset = nullptr;

    if (dataset_storage != nullptr)
        delete[] dataset_storage, dataset_storage = nullptr;

    return returnValue;
}


int handleArgument(int argc, char **argv, Params &params) {
    po::options_description desc("Options");
    try
    {
        std::vector<std::string> sentence;

        /** Define and parse the program options
         */
        namespace po = boost::program_options;

        desc.add_options()
                ("help", "produce help message")
                (",o", po::value<string>(), "output filename, if specified a file with this name containing all of the peers stats is written")
                ("pr", po::value<double>(), "precision for raster")
                ("thr",po::value<int>(), "threshold for raster")
                ("minsize",po::value<int>(), "min size for raster")
                (",i",po::value<string>(), "file name containing dataset")
                ;

        po::variables_map vm;
        try
        {
            po::store(po::parse_command_line(argc, argv, desc), vm); // throws on error

            /** --help option
             */
            if (vm.count("help")) {
                cerr << desc << endl;
                cerr.flush();
                return argumentError(__FUNCTION__);
            }
            po::notify(vm); /// throws on error, so do after help in case there are any problems

        } catch(boost::program_options::error& e) { /// in case of unrecognised options
            cerr << "ERROR: " << e.what() << std::endl << std::endl;
            cerr << desc << endl;
            cerr.flush();
            return argumentError(__FUNCTION__);
        }


        if ( vm.count("-o") )
            params.outputFilename = vm["-o"].as<string>();


        if ( vm.count("-i") )
            params.name_file = vm["-i"].as<string>();

        if ( vm.count("pr") )
            params.precision = vm["pr"].as<double>();

        if ( vm.count("thr") )
            params.threshold = vm["thr"].as<int>();

        if ( vm.count("minsize") )
            params.min_size = vm["minsize"].as<int>();


    } catch(std::exception& e) {
        cerr <<"ERROR: " << e.what() << endl;
        return argumentError(__FUNCTION__);
    }

    return 0;

}

int initParams(Params &params) {
    params.name_file =          DEFAULT_NAME_FILE;
    params.outputFilename =     DEFAULT_OUTPUTFILENAME;
    params.precision =          DEFAULT_PRECISION;
    params.threshold =          DEFAULT_THRESHOLD;
    params.min_size =           DEFAULT_MIN_SIZE;
    return 0;
}

