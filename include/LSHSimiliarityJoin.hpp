

#include <iostream>
#include <sstream>
#include <map>
#include <array>
#include <vector>
#include <random>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h> 
#include "ff/dff.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/tuple.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/array.hpp"
#include "cereal/types/utility.hpp"
#include "configParser.hpp"

#ifdef _USE_METALL_
#include "mmap_allocator.hpp"
#endif

//#define SIM_THRESHOLD 10 //0.01 for taxi  //generated 10
//#define LSH_SEED 234
//#define LSH_RESOLUTION 80 //0.08 for taxi // generated 80
#define MAX_TRAJECTORIES_PER_NODE 10000
//#define PROCESSES 2
//#define MAPPER 2
//#define REDUCER 2

//#define BATCHING_THRESHOLD 512

//#define WRITE_RESULT

//#define PROFILE_DATA

std::atomic<size_t> founded = 0;
std::atomic<size_t> globalCounter = 0;
struct membuf : public std::streambuf {
    membuf(char* start, size_t size) {
        this->setg(start, start, start + size);
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) {
        if (dir == std::ios_base::cur)
            gbump(off);
        else if (dir == std::ios_base::end)
            setg(eback(), egptr() + off, egptr());
        else if (dir == std::ios_base::beg)
            setg(eback(), eback() + off, egptr());
        return gptr() - eback();
    }
};

std::istream& GotoLine(std::istream& file, unsigned int num){
    file.clear(); file.seekg(0, file.beg);
    for(int i=0; i < num - 1; ++i)
        file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    return file;
}

std::string leadingZeros(int acct_no) {
  char buffer[9];
  std::snprintf(buffer, sizeof(buffer), "%02d", acct_no);
  return buffer;
}

template<typename T>
struct item {
    size_t id;
    int dataset;
    T content;
};

template<typename T>
using readFunction_t = std::function<item<T>(std::string&)>;

template<typename T>
using lshFunction_t = std::function<long(const T&)>;

template<typename T>
using similarityFunction_t = std::function<bool(const T&, const T&)>;

template<typename T, size_t _lshs>
class SimilarityJoin {
    int processes;
    std::string inputFileName;
    readFunction_t<T> reader;
    std::array<lshFunction_t<T>, _lshs> lshFamily;
    similarityFunction_t<T> similarityTest;
    std::hash<long> hash;
    char* mapped;
    size_t size;
    ff::ff_a2a a2a;

    //using R2M_feedback_t = std::map<long, std::pair<size_t, size_t>>;
    struct R2M_feedback_t {
        std::unordered_map<long, std::pair<size_t,size_t>> m;

        template<class Archive>
        void serialize(Archive & archive) {
            archive(m);
        }

        size_t getSize(){
            return m.size()*3*sizeof(long);
        }
    };

    struct MiForwarder : ff::ff_minode_t<R2M_feedback_t> {
        ssize_t histogramReceived = -1;
        size_t reducers;
        MiForwarder(size_t reducers) : reducers(reducers) {}
        R2M_feedback_t* svc(R2M_feedback_t* i) {
            this->ff_send_out(i);
            if (++histogramReceived == reducers) {
                return this->EOS;
            }
            return this->GO_ON;
        }
    };

    struct M2R {
        long LSH;
        int dataSet;
        T trajectory;
        std::array<long, _lshs> relativeLSHs;
        long id = -1;

        template<class Archive>
        void serialize(Archive & archive) {
            archive(LSH, dataSet, id);
	        if (id != -1) archive(relativeLSHs, trajectory);
        }
    };

    class Mapper : public ff::ff_monode_t<R2M_feedback_t, std::vector<M2R>> {
        std::unordered_map<long, std::pair<size_t, size_t>> histograms;
        membuf membuff;
        std::istream in;
        size_t startLine, lineToProcess;
        std::streampos initialPosition;
        SimilarityJoin* parent;
        std::vector<std::vector<M2R>*> outBuffers;

    public:    
        inline static std::atomic<int> mapperExited;
	Mapper(membuf membuf_, size_t lineStart, size_t lineToProcess, SimilarityJoin* parent) : membuff(membuf_), in(&membuff), startLine(lineStart), lineToProcess(lineToProcess), parent(parent) { }

        int svc_init(){

            outBuffers.resize(this->get_num_outchannels());
            for(size_t i  = 0; i < this->get_num_outchannels(); ++i){
                outBuffers[i] = new std::vector<M2R>();
                outBuffers[i]->reserve(BATCHING_THRESHOLD);
            }

            GotoLine(in, startLine+1); // got to the correct line
            this->initialPosition = in.tellg();
            return 0;
        }

        void sendOutTo(M2R item, size_t dest){
	        auto* buff = outBuffers[dest];
            buff->push_back(std::move(item));
            if (buff->size() == BATCHING_THRESHOLD){
                this->ff_send_out_to(buff, dest);
                outBuffers[dest] = new std::vector<M2R>();
                outBuffers[dest]->reserve(BATCHING_THRESHOLD);
            }
        }

        void flushBuffers(){
            for(size_t i  = 0; i < this->get_num_outchannels(); ++i){
                this->ff_send_out_to(outBuffers[i], i);
                outBuffers[i] = new std::vector<M2R>();
                outBuffers[i]->reserve(BATCHING_THRESHOLD);
            }

#ifndef DISABLE_FF_DISTRIBUTED                
            this->ff_send_out(this->FLUSH);
#endif
        }


        std::vector<M2R>* svc(R2M_feedback_t* input){
            if (input == nullptr){ // Phase 1: Emitting key,value for computing histograms 
                std::string line;
                for(int i = 0; i < lineToProcess; i++){
                    std::getline(in, line);
                    if (in.eof()) break; // EOF check
                    struct item<T> it = parent->reader(line); // call the user define function to parse a line of the dataset

                    for (auto& lsh_f : parent->lshFamily){
                        M2R out; long lsh = lsh_f(it.content);
                        out.LSH = lsh;
			out.id = -1;
                        out.dataSet = it.dataset;
                        this->sendOutTo(std::move(out), parent->hash(lsh) % this->get_num_outchannels());
                        //this->ff_send_out_to(out, parent->hash(out->LSH) % this->get_num_outchannels());
                    }
                }
                ff::cout << "[Mapper][Phase 1] File readed successfully!\n";
                
                // sending logical eos to the reducer to switch phase!
                M2R eos; eos.dataSet = -1;
                broadcast(eos, 0, this->get_num_outchannels());
                this->flushBuffers();
                return this->GO_ON;
            }
            // union of all the received partial histograms into local histograms!
            histograms.merge(input->m);
            //histograms.insert(input->m.begin(), input->m.end());
            delete input;
            return this->GO_ON;
        }

        void broadcast(M2R& task, size_t start, size_t length){
            for(size_t i = start; i < start+length; i++)
                this->sendOutTo(task, i % this->get_num_outchannels());
                //this->ff_send_out_to(new M2R(*task), i % this->get_num_outchannels());
            //delete task;
        }

        void unicastRandom(M2R& task, size_t start, size_t length){
            this->sendOutTo(task, (start + (rand() % length)) % this->get_num_outchannels());
            //this->ff_send_out_to(task, (start + (rand() % length)) % this->get_num_outchannels());
        }

        void eosnotify(ssize_t){

            // i receive just one EOS because i'm under a multi input forwarder channel
            ff::cout << "[Mapper][Phase 2] Received all the histograms\n";
	    ff::cout << "Histogram size: " << histograms.size() << std::endl;
            in.clear(); in.seekg(initialPosition, in.beg);
            std::string line;
            for(int i = 0; i < lineToProcess; i++){
                std::getline(in, line);
                if (in.eof()) {
                    break;
                } // EOF check
                struct item<T>&& it = parent->reader(line); // call the user define function to parse a line of the dataset
                std::array<long, _lshs> LSHs;
                for(int index = 0; index < _lshs; index++)
                    LSHs[index] = parent->lshFamily[index](it.content);
                /*for(auto& lsh_f : parent->lshFamily)
                    LSHs.push_back(lsh_f(it.content));*/

                for (auto l : LSHs){
                    if (!histograms.count(l)) // skipping emitting lsh function that is not present in histogram
                        continue;
                    M2R out;
                    out.LSH = l;
                    out.dataSet = it.dataset;
                    out.trajectory = it.content;
                    out.id = it.id;
                    out.relativeLSHs = LSHs;
                    
                    auto& histogram = histograms[l];


                    auto reducerR = ceil((double)histogram.first / (double) MAX_TRAJECTORIES_PER_NODE);
                    auto reducerS = ceil((double)histogram.second / (double) MAX_TRAJECTORIES_PER_NODE);
                    if (reducerR > this->get_num_outchannels()) reducerR = this->get_num_outchannels();
                    if (reducerS > this->get_num_outchannels()) reducerS = this->get_num_outchannels();
                    auto reducer_base = parent->hash(l) % this->get_num_outchannels();

                    if (histogram.first > histogram.second){ // if from S broadcast to all, if from R send to just one
                        if (out.dataSet == 0) unicastRandom(out, reducer_base, reducerR);
                        else broadcast(out, reducer_base, reducerR);
                    } else {
                        if (out.dataSet == 1) unicastRandom(out, reducer_base, reducerS);
                        else broadcast(out, reducer_base, reducerS);
                    }
                }
            }
            this->flushBuffers();
            this->ff_send_out(this->EOS);
        }

        void svc_end(){
            if (--mapperExited == 0) parent->freeMemoryMappedArea();
        }

    };

    //using R2M_feedback_w_t = std::pair<R2M_feedback_t*, size_t>;

    struct R2M_feedback_w_t {
        R2M_feedback_w_t(){}
        R2M_feedback_w_t(R2M_feedback_t* f, size_t s): first(f), second(s) {}
        R2M_feedback_t* first;
        size_t second;

        template<class Archive>
        void serialize(Archive & archive){
            std::cerr << "WARNING!!!!!!!! If you see this there is probably an ERROR!!!\n";
            archive(second);
        }
    };
    

    class MOForwarder : public ff::ff_monode_t<R2M_feedback_w_t, R2M_feedback_t>{
        R2M_feedback_t* svc(R2M_feedback_w_t* in){
            this->ff_send_out_to(in->first, in->second);
#ifndef DISABLE_FF_DISTRIBUTED
            this->ff_send_out(this->FLUSH);
#endif
            delete in;
            return this->GO_ON;
        }
    };

    class Reducer : public ff::ff_minode_t<std::vector<M2R>, R2M_feedback_w_t> {
        bool histogram_phase;
        size_t histogramEOS = 0;

        std::unordered_map<long, std::tuple<size_t, size_t, std::unordered_set<size_t>>> localHistogram;

#ifdef _USE_METALL_
std::unordered_map<long, std::vector<M2R>> elements_cache;
inline static std::atomic<size_t> cache_size = 0;
mm::unordered_map<long, mm::vector<M2R>> elements;
size_t sizeOfVector = 9;
size_t sizeOfMemory = 103079215104;
#else
std::unordered_map<long, std::vector<M2R>> elements;
#endif
        size_t neos = 0;
        SimilarityJoin* parent;
        size_t foundSimilar = 0;
        size_t Nmappers;
        std::ofstream outputFile;
    public:
        Reducer(size_t mappers, SimilarityJoin* parent) : Nmappers(mappers), histogram_phase(true), localHistogram(), elements(), parent(parent) {}

        int svc_init(){
#ifdef WRITE_RESULT
            outputFile.open(ff::DFF_getMyGroup() + "out-" + std::to_string(this->get_my_id()));
#endif
            return 0;
        }

        void sendBackHistograms(){
            std::vector<R2M_feedback_t*> filteredHistograms(Nmappers);
            for(size_t i = 0; i < Nmappers; i++)
                filteredHistograms[i] = new R2M_feedback_t;
            
            for (auto& [lsh, tuple_] : localHistogram)
                if (std::get<0>(tuple_) > 0 && std::get<1>(tuple_) > 0) // filter out lsh that does not have both positive counters
                    for (auto i : std::get<2>(tuple_))
                        filteredHistograms[i]->m.operator[](lsh) = std::make_pair(std::get<0>(tuple_), std::get<1>(tuple_));

            for(size_t i = 0; i < Nmappers; i++)
                this->ff_send_out(new R2M_feedback_w_t(filteredHistograms[i], i));
        }

        R2M_feedback_w_t* svc(std::vector<M2R>* in){
            if (histogram_phase){
                for(auto& input : *in){
                    if (input.dataSet == -1){ // Histogram EOS received
                        if (++histogramEOS == Nmappers){
                            ff::cout << "[Reducer][Phase 1] Sending back histograms! [Size: "<< localHistogram.size() <<"]\n";
                            sendBackHistograms();
                            histogram_phase = false;
                            localHistogram.clear(); // cleanup the unused memory
                        #ifdef _USE_METALL_
                            elements.reserve(localHistogram.size()); // reserve memory for elements based on the local histograms
                            
                            // memory optimization
                            for(auto& [lsh, tupl] : localHistogram)
                                elements[lsh].reserve(std::get<0>(tupl)+std::get<1>(tupl));
                        #endif
                        }
                        return this->GO_ON;
                    }

                    auto localHistogram_it = this->localHistogram.find(input.LSH);
                    if (localHistogram_it == this->localHistogram.end())
                        localHistogram[input.LSH] = std::make_tuple<size_t, size_t, std::unordered_set<size_t>>(input.dataSet == 0, input.dataSet == 1, std::unordered_set<size_t>({(size_t) this->get_channel_id()}));
                    else {
                        auto& tuple_ = localHistogram[input.LSH];
                        input.dataSet == 0 ? std::get<0>(tuple_)++ : std::get<1>(tuple_)++;
                        std::get<2>(tuple_).insert(this->get_channel_id()); // save from which mapper i got the input item
                    }
                }

                delete in;
                return this->GO_ON;
            }

#ifndef _USE_METALL_
            for(auto& input : *in) elements[input.LSH].push_back(std::move(input));
#else
         for(auto& input : *in){
                std::vector<M2R>& cache_vector = elements_cache[input.LSH];
                cache_vector.push_back(input);

                //auto found = std::find_if(cache_map.begin(), cache_map.end(), [&](auto& e){return e.first == input.LSH;});
                //if (found == cache_map.end()) cache_map.push_back({input.LSH, 1});
                //else found->second++;

                cache_size += sizeof(M2R);

                if (cache_vector.size() == sizeOfVector){
                    for (auto& e : cache_vector){
                        elements[input.LSH].push_back(e);
			cache_vector.erase(cache_vector.begin(), cache_vector.end());
			//assert(cache_vector.capacity() == 0);
			cache_size -= sizeof(M2R);
                    }
		    /*auto nh = elements_cache.extract(input.LSH);
	            for(auto& e : nh.mapped()) elements[input.LSH].push_back(e);	    
		    cache_size -= sizeOfVector*sizeof(M2R);*/
                }

                if (cache_size >= sizeOfMemory){ 
          
                    for(auto& [k,v] : elements_cache){
                        cache_size -= v.size()*sizeof(M2R);
                        for(auto& e : v)
                            elements[k].push_back(e);
                        v.erase(v.begin(), v.end());
                        if (cache_size <= 0.8*sizeOfMemory) break;
                    }

		  /* while(true){
			auto nh = elements_cache.extract(elements_cache.begin());
		    	for(auto& e : nh.mapped()) elements[nh.key()].push_back(e);	
			cache_size -= nh.mapped().size()*sizeof(M2R);
			if (cache_size <= 0.8*sizeOfMemory) break;
		    }	 */   

                }
            }     
#endif              
            delete in;
            return this->GO_ON;
        }

        inline void similarity(const long lsh, const M2R& a, const M2R& b){
            for(size_t ii = 0; ii < a.relativeLSHs.size(); ii++)
                    if (a.relativeLSHs[ii] == b.relativeLSHs[ii]){
                        if (lsh == a.relativeLSHs[ii]){
                            if (parent->similarityTest(a.trajectory, b.trajectory)){
                                ++foundSimilar;
#ifdef WRITE_RESULT
                                this->outputFile << a.id << "\t" << b.id << std::endl;
#endif
                            }
                        }
                        return;
                    }
        }

        void svc_end(){
            std::cout << "REducer SVC_END!\n";
#ifdef _USE_METALL_
            for(auto& [key, vector] : elements_cache)
                if (vector.size() != 0){
                    for(auto& e: vector)
                        elements[key].push_back(e);
		        vector.erase(vector.begin(), vector.end());
		    
            }
            {std::unordered_map<long, std::vector<M2R>>().swap(elements_cache);}
#endif
            for (auto& [lsh, elements_v] : elements){
                // compute all possilbe combination of elements in elements_v vector
                for(size_t i = 0; i < elements_v.size(); i++){
                    for(size_t j = i+1; j < elements_v.size(); j++){
                        if (elements_v[i].dataSet != elements_v[j].dataSet)
                            similarity(lsh, elements_v[i], elements_v[j]);
                    }
                }
            }
            founded += foundSimilar;

#ifdef WRITE_RESULT
            this->outputFile.close();
#endif

        }
    };


public:
    SimilarityJoin(std::string configFile, 
                   std::string chunkFileName,
                   size_t lines,
                   readFunction_t<T> readerFunction, 
                   std::array<lshFunction_t<T>, _lshs> lshFamily, 
                   similarityFunction_t<T> simFunction
                  ) : reader(readerFunction), 
                      lshFamily(lshFamily), 
                      similarityTest(simFunction) {
#ifdef _USE_METALL_
        metall_open(METALL_CREATE_ONLY, "/tmp");
#endif
        int fd = open(chunkFileName.c_str(), O_RDONLY);
        struct stat s;
        fstat (fd, &s);
        size = s.st_size;
        mapped = reinterpret_cast<char*>(mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0));
        close(fd);
        std::vector<ff::ff_node*> mappers, reducers;

	auto configValues = parseConfigFile(configFile);
        int mappersTotal = 0, reducersTotal = 0;

        for(int i = 0; i < configValues.size(); i++){
            mappersTotal += std::stoi(configValues[i][1]);
            reducersTotal += std::stoi(configValues[i][2]);
        }

	int executingGroup = std::atoi(ff::DFF_getMyGroup().c_str()+1);
	Mapper::mapperExited = std::stoi(configValues[executingGroup][1]);

        for (int p = 0; p < configValues.size(); p++){
            auto g = a2a.createGroup("G"+std::to_string(p));
            int myMappers = std::stoi(configValues[p][1]);
            int myReducers = std::stoi(configValues[p][2]);
            size_t lineToProcessXMapper = lines / myMappers;
            for (int i = 0; i < myMappers; i++){
                auto* component = new ff::ff_comb(new MiForwarder(reducersTotal), new Mapper(std::move(membuf(mapped, size)), i*lineToProcessXMapper, (i == myMappers-1 ? lines-i*lineToProcessXMapper : lineToProcessXMapper), this), true, true);
                mappers.push_back(component);
                g << component;
            }

            for (int i = 0; i < myReducers; i++){
                auto* component = new ff::ff_comb(new Reducer(mappersTotal, this), new MOForwarder, true, true);
                reducers.push_back(component);
                g << component;
            }
        }


        a2a.add_firstset(mappers, 0, true);
        a2a.add_secondset(reducers, true);
    }


    int execute(){
#if 0        
        ff::ff_pipeline pipe;
        pipe.add_stage(&a2a);
        pipe.wrap_around();
        return pipe.run_and_wait_end();
#else
        a2a.wrap_around();
        return a2a.run_and_wait_end();
#endif
    }

    void freeMemoryMappedArea(){
        munmap(mapped, size);
    }

    ~SimilarityJoin(){
        
    }

};
