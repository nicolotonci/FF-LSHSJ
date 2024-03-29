CXX	       +=-std=c++20 # -DDFF_EXCLUDE_MPI
INCLUDES	= -I ./ -I ../../include/ 

ifdef CEREAL_HOME
    INCLUDES     += -I$(CEREAL_HOME)/include
else
    INCLUDES     += -I ~/cereal/include
endif

ifdef FF_HOME
    INCLUDES     += -I$(FF_HOME)
else
    INCLUDES     += -I ~/fastflow
endif

ifdef DEBUG
    OPTIMIZE_FLAGS  += -g -fno-inline-functions -DPRINT_IR
else
    OPTIMIZE_FLAGS  += -O3 -finline-functions -DNDEBUG
endif

ifdef SINGLE_PROCESS
OPTIMIZE_FLAGS += -DDISABLE_FF_DISTRIBUTED
endif

ifdef BATCH
OPTIMIZE_FLAGS += -DBATCHING_THRESHOLD=$(BATCH)
else
OPTIMIZE_FLAGS += -DBATCHING_THRESHOLD=256
endif

ifdef BOUNDED
OPTIMIZE_FLAGS += -DFF_BOUNDED_BUFFER -DDEFAULT_BUFFER_CAPACITY=1 
endif

ifdef WRITE_RESULT
OPTIMIZE_FLAGS += -DWRITE_RESULT
endif

ifdef NONBLOCKING
OPTIMIZE_FLAGS += -DDFF_EXCLUDE_BLOCKING
endif

ifdef NO_DEFAULT_MAPPING
OPTIMIZE_FLAGS += -DNO_DEFAULT_MAPPING
endif

ifdef MAX_TRAJECTORIES_REDUCER
OPTIMIZE_FLAGS += -DMAX_TRAJECTORIES_PER_NODE=$(MAX_TRAJECTORIES_REDUCER)
endif

ifdef USE_METALL
# REQUIRES: boost, metall (headers), metall (compiled C interface)
INCLUDES += -I ~/boost/include -I~/metall/include
OPTIMIZE_FLAGS +=  -DMETALL_INITIAL_SEGMENT_SIZE=4294967296 -D_USE_METALL_
LDFLAGS += libmetall_c.a 
endif

LDFLAGS 	+= -pthread
OBJS            = frechet_distance.o frechet_distance2.o geometry_basics.o
TARGETS		= main

.PHONY: all clean cleanall
.SUFFIXES: .cpp 

main: main.cpp $(OBJS) datasetReader.hpp frechet_distance.hpp geometry_basics.hpp
%: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTIMIZE_FLAGS) -o $@ $< $(OBJS) $(LDFLAGS) $(LIBS)

all		: $(TARGETS)
clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~
