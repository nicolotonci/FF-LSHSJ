//#define DISABLE_FF_DISTRIBUTED
#define PROCESSES 2


#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <random>
#include "LSHSimiliarityJoin.hpp"
#include "datasetReader.hpp"
#include "hash.hpp"
#include "geometry_basics.hpp"
#include "frechet_distance.hpp"




//#define SIM_THRESHOLD 10 
#define LSH_SEED 234
#define LSH_RESOLUTION 0.08 //0.08 for taxi // generated 80
//#define MAX_TRAJECTORIES_PER_NODE 10000

//#define MAPPER 2
//#define REDUCER 2

//#define BATCHING_THRESHOLD 512

//#define WRITE_RESULT


const static double SIM_THRESHOLD = 0.01;  //0.01 for taxi  //generated 10
const static double SIM_THRESHOLD_SQR = sqr(SIM_THRESHOLD);

int main(int argc, char** argv){

    ff::DFF_Init(argc, argv);

    using data_t = curve;

    FrechetLSH lsh1, lsh2, lsh3, lsh4, lsh5, lsh6, lsh7, lsh8;
    lsh1.init(LSH_RESOLUTION, 1*LSH_SEED, 1);
    lsh2.init(LSH_RESOLUTION, 2*LSH_SEED, 2);
    lsh3.init(LSH_RESOLUTION, 3*LSH_SEED, 3);
    lsh4.init(LSH_RESOLUTION, 4*LSH_SEED, 4);
    lsh5.init(LSH_RESOLUTION, 5*LSH_SEED, 5);
    lsh6.init(LSH_RESOLUTION, 6*LSH_SEED, 6);
    lsh7.init(LSH_RESOLUTION, 7*LSH_SEED, 7);
    lsh8.init(LSH_RESOLUTION, 8*LSH_SEED, 8);

#ifndef DISABLE_FF_DISTRIBUTED
    std::string inputFile = std::string(argv[2]) + "-" + leadingZeros(std::stoi(ff::DFF_getMyGroup().substr(1)));
#else
    std::string inputFile = std::string(argv[2]); 
#endif

    SimilarityJoin<data_t, 8> pippo(argv[1], // config File
    std::string(inputFile), // filename input
    std::atoll(argv[3]), // lines count
    [](std::string& line) -> item<data_t> {  // parse Function
        std::istringstream ss(line);
        item<data_t> output;
        ss >> output.id;
        ss >> output.dataset;
        std::string tmp;
        ss >> tmp;
        if (!(tmp.find_first_of("[")==std::string::npos)) {
			tmp.replace(0, 1, "");
			tmp.replace(tmp.length() - 1, tmp.length(), "");
			bool ext = false;
			while (!ext) {
				std::string extrait = tmp.substr(tmp.find("["), tmp.find("]") + 1);
				std::string extrait1 = extrait.substr(1, extrait.find(",") - 1);
				std::string extrait2 = extrait.substr(extrait.find(",") + 1);
				extrait2 = extrait2.substr(0, extrait2.length() - 1);
				double e1 = stod(extrait1);
				double e2 = stod(extrait2);
				output.content.push_back(std::move(point(e1, e2)));
				ext = (tmp.length() == extrait.length());
				if (!ext)
					tmp = tmp.substr(tmp.find_first_of("]") + 2, tmp.length());
			}
		}
        return output;
    }, 
    {   [&](const data_t& i){return lsh1.hash(i);},  // lsh function family
        [&](const data_t& i){return lsh2.hash(i);}, 
        [&](const data_t& i){return lsh3.hash(i);}, 
        [&](const data_t& i){return lsh4.hash(i);}, 
        [&](const data_t& i){return lsh5.hash(i);}, 
        [&](const data_t& i){return lsh6.hash(i);}, 
        [&](const data_t& i){return lsh7.hash(i);}, 
        [&](const data_t& i){return lsh8.hash(i);}
    },
    [](const data_t& c1, const data_t& c2){
        // check euclidean distance
        if (euclideanSqr(c1[0], c2[0]) > SIM_THRESHOLD_SQR || euclideanSqr(c1.back(), c2.back()) > SIM_THRESHOLD_SQR) 
            return false;
        // check equal time
        if (equalTime(c1, c2, SIM_THRESHOLD_SQR) || get_frechet_distance_upper_bound(c1, c2) <= SIM_THRESHOLD)
            return true;
        if (negfilter(c1, c2, SIM_THRESHOLD))
            return false;
        // full check 
        if (is_frechet_distance_at_most(c1, c2, SIM_THRESHOLD))
            return true;
        return false;
    }); // distance between two item function


    ffTime(ff::START_TIME);
    pippo.execute();
    ffTime(ff::STOP_TIME);

    std::cout << "Similar trajectories " << founded << std::endl;
    std::cout << "Time elapsed " << ffTime(ff::GET_TIME)/1000.0 << std::endl;
    return 0;
}
