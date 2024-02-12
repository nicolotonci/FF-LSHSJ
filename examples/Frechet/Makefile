CXX	       +=-std=c++20 -DDFF_EXCLUDE_MPI -I /Users/nicolotonci/repos/metall/include -I ./
INCLUDES	= -I ~/fastflow -I ~/cereal/include 

ifdef DEBUG
    OPTIMIZE_FLAGS  += -g -fno-inline-functions -DPRINT_IR
else
    OPTIMIZE_FLAGS  += -O3 -finline-functions -DNDEBUG
endif

ifdef MAPPER
OPTIMIZE_FLAGS += -DMAPPER=$(MAPPER)
else
OPTIMIZE_FLAGS += -DMAPPER=2
endif

ifdef REDUCER
OPTIMIZE_FLAGS += -DREDUCER=$(REDUCER)
else
OPTIMIZE_FLAGS += -DREDUCER=2
endif

ifdef BATCH
OPTIMIZE_FLAGS += -DBATCHING_THRESHOLD=$(BATCH)
else
OPTIMIZE_FLAGS += -DBATCHING_THRESHOLD=256
endif

ifdef BOUNDED
OPTIMIZE_FLAGS += -DFF_BOUNDED_BUFFER -DDEFAULT_BUFFER_CAPACITY=1 
endif

ifdef NONBLOCKING
OPTIMIZE_FLAGS += -DDFF_EXCLUDE_BLOCKING
endif


ifdef USE_METALL
OPTIMIZE_FLAGS += -I /usr/local/Cellar/boost/1.82.0_1/include -I /home/prof/tonci/metall/include -DMETALL_INITIAL_SEGMENT_SIZE=4294967296 -D_USE_METALL_
LDFLAGS += libmetall_c.a
endif

CXXFLAGS  	= # -DNO_DEFAULT_MAPPING -DBLOCKING_MODE -DFF_BOUNDED_BUFFER
LDFLAGS 	+= -pthread
OBJS            = dependencies/frechet_distance.o dependencies/frechet_distance2.o dependencies/geometry_basics.o
TARGETS		= similarity_app interface_batch_O #impl interface interface_batch  interface_batch_OMMAP interface_batch_OM similarity_app

.PHONY: all clean cleanall
.SUFFIXES: .cpp 


#impl: impl.cpp $(OBJS) datasetReader.hpp dependencies/frechet_distance.hpp dependencies/geometry_basics.hpp
#interface: interface.cpp $(OBJS) datasetReader.hpp dependencies/frechet_distance.hpp dependencies/geometry_basics.hpp
#interface_batch: interface_batch.cpp $(OBJS) datasetReader.hpp dependencies/frechet_distance.hpp dependencies/geometry_basics.hpp
interface_batch_O: interface_batch_O.cpp $(OBJS) datasetReader.hpp dependencies/frechet_distance.hpp dependencies/geometry_basics.hpp
#interface_batch_OMMAP: interface_batch_OMMAP.cpp $(OBJS) datasetReader.hpp dependencies/frechet_distance.hpp dependencies/geometry_basics.hpp
#interface_batch_OM: interface_batch_OM.cpp $(OBJS) datasetReader.hpp dependencies/frechet_distance.hpp dependencies/geometry_basics.hpp
similarity_app: similarity_app.cpp $(OBJS) datasetReader.hpp dependencies/frechet_distance.hpp dependencies/geometry_basics.hpp
%: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTIMIZE_FLAGS) -o $@ $< $(OBJS) $(LDFLAGS) $(LIBS)

all		: $(TARGETS)
clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~



