CXX	       +=-std=c++17

ifdef DEBUG
    OPTIMIZE_FLAGS  += -g -fno-inline-functions -DPRINT_IR
else
    OPTIMIZE_FLAGS  += -O3 -finline-functions -DNDEBUG
endif

TARGETS		= dataset-indexer

.PHONY: all clean cleanall
.SUFFIXES: .cpp 

dataset-indexer: dataset-indexer.cpp
%: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTIMIZE_FLAGS) -o $@ $< $(OBJS) $(LDFLAGS) $(LIBS)

all		: $(TARGETS)
clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~
