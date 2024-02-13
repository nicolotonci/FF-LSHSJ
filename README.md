
# FastFlow based Locality Sensitive Hash Similarity Join framework

We introduce a C++-based high-level parallel pattern implemented on top of FastFlow Building Blocks to provide the programmer with ready-to-use similarity join computations. The SimilarityJoin pattern is implemented according to the MapReduce paradigm enriched with Locality Sensitive Hashing (LSH) to optimize the whole computation. The new parallel pattern can be used with any C++ serializable data structure and executed on shared- and distributed-memory machines.

## Dependencies
All the dependencies must be configured in the Makefile in example folders in order to compile correctly.

 - **FastFlow** (DistributedFF4Similarities branch) - REQUIRED
```
git clone --branch DistributedFF4Similarities https://github.com/fastflow/fastflow
```
 - **Cereal** C++ serialization library - REQUIRED
```
git clone https://github.com/USCiLab/cereal
```
 - **METALL**  persistent Memory Allocator for Data-Centric Analytics - OPTIONAL
(Requires BOOST library in order to be compiled)
```
git clone https://github.com/LLNL/metall
cd metall 
```
Edit the file *metall/include/metall/defs.hpp* to customize the metall parameters. For example we set *METALL_SEGMENT_BLOCK_SIZE* to 4GB.
```
mkdir build 
cd build 
cmake .. -DBOOST_ROOT=/path/to/boost/root/ -DBUILD_C=ON
make
```
Link the *libmetall_c.a* to your executable editing the Makefile in example folders.

## Compile time options
|Makefile Variable| Feature |
|--|--|
| DEBUG | Turns ON debugging code. |
| SINGLE_PROCESS | Optimize the execution for shared memory. To execute a single process turn this option ON.|
| BOUNDED | Makes the FastFlow queue bounded to a capacity equal to 1. This is useful to save memory and optimizing the execution for very large datasets.|
| NONBLOCKING | Set the FastFlow execution mode to non-blocking to privilege node-responsiveness with respect to power efficiency.|
|NO_DEFAULT_MAPPING| Disable the default mapping of threads specified with the FastFlow mapping_string.sh command, leaving control of the mapping to the operating system kernel.|
| USE_METALL | Enable Metall , to perform spill-on-disks, in case of insufficient memory.|

## Configuration files
Framework configuration file is a csv file with space separated values. The first column is the hostname of the machine in which the i-th group is going to run, the second column specify the number of mappers for that group and the last column to the same for the number of reducers. The structure is the following:

    hostname #mappers #reducers
For example:

    compute1 10 5
    compute2 5 15
Those configuration file must be converted to FastFlow json configuration files through the Python script *util/ff-config-builder.py*. The output file can be further customized to optimize your execution. The same file can be used also to launch the execution through the FastFlow *dff_run* utility.
