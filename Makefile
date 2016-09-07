CXX=g++
#CXXFLAGS=-O0 -g --std=c++14 -Werror -Wall -Wpedantic -Iinclude/
CXXFLAGS=-O3 --std=c++14 -Werror -Wall -Wpedantic -Iinclude/ -DNDEBUG

TESTS=$(wildcard testdata/*)
OUTPUT=$(TESTS:testdata/%=tests/%)

all: lz78 lzw

lz78: src/lz78.cpp src/log.h include/lz78/code.h include/lz78/lz78.h include/bitstream.h src/common.h include/dictionary.h include/adaptive_huffman.h
	$(CXX) $(CXXFLAGS) -o lz78 src/lz78.cpp

lzw: src/lzw.cpp src/log.h include/lzw/code.h include/lzw/lzw.h include/bitstream.h src/common.h include/dictionary.h include/adaptive_huffman.h
	$(CXX) $(CXXFLAGS) -o lzw src/lzw.cpp

tests/%: testdata/%
	for i in `seq 15 31`; do time ./lz78 -b $${i} -f $<; mv $<.lz78 $@_lz78.$${i}.lz78; time ./lz78 -b $${i} -df $@_lz78.$${i}.lz78; cmp $< $@_lz78.$${i}; done
	for i in `seq 15 31`; do time ./lzw -b $${i} -f $<; mv $<.lzw $@_lzw.$${i}.lzw; time ./lzw -b $${i} -df $@_lzw.$${i}.lzw; cmp $< $@_lzw.$${i}; done

tests: $(OUTPUT)

clean:
	-rm -f lz78 lzw tests/* testdata/*.lzw testdata/*.lz78
