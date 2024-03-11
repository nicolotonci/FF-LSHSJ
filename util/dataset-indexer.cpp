#include <iostream>
#include <fstream>
#include <limits>
#include <chrono>

int main(int argc, char* argv[]) {

    auto start = std::chrono::high_resolution_clock::now();

    int partitions = std::atoi(argv[1]);
    //std::string dataSetPath = std::string(argv[2]);
    std::string indexPath = std::string(argv[3]);

    std::streampos positions[partitions];

    size_t total_lines = 0;
    std::string line;

    std::cout << "Opening: " << argv[2] << std::endl;

    std::ifstream dataset(argv[2]);

     if (!dataset.is_open()){
        std::cout << "failed to open the input file!\n";
	return 1;
    }

    while(!dataset.eof()){
        dataset.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        total_lines++;
    }
    total_lines--; // required since we always count one more line


    std::cout << "Total lines of the dataset: " << total_lines << std::endl;
    if (total_lines < partitions) {
	std::cout << "ERROR: Partitions are more than lines in the input dataset\n";
	return 0;
    }
    dataset.clear(); dataset.seekg(0, dataset.beg);
    size_t linesPerPartition = total_lines / partitions;

    std::cout << "Lines per partitin: " << linesPerPartition << std::endl;

    for(int i = 0; i < partitions; i++){
        positions[i] = dataset.tellg();
        for(size_t l = 0; l < linesPerPartition; l++)
            dataset.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    }

    std::ofstream output(argv[3]);
    for (int i = 0; i < partitions; i++)
        output << positions[i] << '\t' << (i+1 == partitions ? (total_lines - i*linesPerPartition) : linesPerPartition) <<   std::endl;

    std::cout << "Executed in: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() << " ms\n";
    return 0;
 }
