##
##

CXX	=g++
##
## Use our standard compiler flags for the course...
## You can try changing these flags to improve performance.
##
CXXFLAGS_DEBUG = -g -O0 -fno-omit-frame-pointer -Wall
CXXFLAGS_FAST  = -O3 -march=native -mtune=native -fomit-frame-pointer -funroll-loops -DNDEBUG -Wall


MODE ?= fast

ifeq ($(MODE),fast)
  CXXFLAGS = $(CXXFLAGS_FAST)
else
  CXXFLAGS = $(CXXFLAGS_DEBUG)
endif

SRC = FilterMain.cpp Filter.cpp cs1300bmp.cc
HDR = cs1300bmp.h Filter.h rdtsc.h
BIN = filter

goals: filter judge
	@echo "Done"

filter: FilterMain.cpp Filter.cpp cs1300bmp.cc cs1300bmp.h Filter.h rdtsc.h
	$(CXX) $(CXXFLAGS) -o filter FilterMain.cpp Filter.cpp cs1300bmp.cc


$(BIN): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(SRC)


##
## Parameters for the test run
##
FILTERS = gauss.filter vline.filter hline.filter emboss.filter
IMAGES = boats.bmp blocks-small.bmp
TRIALS = 1 2 3 4

#
# Run the Judge script to compute a score
#
judge: filter
	@echo Running run for performance / scoring information
	./Judge -p ./filter -i blocks-small.bmp
	@echo Now checking if the output is correct. Any errors indicate bugs in your code.
	@cmp filtered-avg-blocks-small.bmp tests/filtered-avg-blocks-small.bmp
	@cmp filtered-emboss-blocks-small.bmp tests/filtered-emboss-blocks-small.bmp
	@cmp filtered-gauss-blocks-small.bmp tests/filtered-gauss-blocks-small.bmp
	@cmp filtered-hline-blocks-small.bmp tests/filtered-hline-blocks-small.bmp

#
# Run the Judge tests on both images and then compare the output to the reference output.
#
# Note you shouldn't use this to compute a score -- it's just for testing
#
test:	filter
	./Judge -p ./filter -i boats.bmp
	./Judge -p ./filter -i blocks-small.bmp
	cmp filtered-avg-blocks-small.bmp tests/filtered-avg-blocks-small.bmp
	cmp filtered-avg-boats.bmp tests/filtered-avg-boats.bmp
	cmp filtered-emboss-blocks-small.bmp tests/filtered-emboss-blocks-small.bmp
	cmp filtered-emboss-boats.bmp tests/filtered-emboss-boats.bmp
	cmp filtered-gauss-blocks-small.bmp tests/filtered-gauss-blocks-small.bmp
	cmp filtered-gauss-boats.bmp tests/filtered-gauss-boats.bmp
	cmp filtered-hline-blocks-small.bmp tests/filtered-hline-blocks-small.bmp
	@echo All tests passed



.PHONY: fast judge-fast test-fast clean

fast:
	$(MAKE) MODE=fast $(BIN)

judge-fast:
	$(MAKE) MODE=fast judge

test-fast:
	$(MAKE) MODE=fast test


clean:
	-rm -f *.o
	-rm -f filter
	-rm -f filtered-*.bmp
