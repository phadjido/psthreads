CXX=g++
CXXFLAGS=-pthread -I../psthread -I../include
CXXLIBS=../psthread/libpsthread.a ../uth/libuth.a

all: ps_thread

ps_thread:ps_thread.cpp
	$(CXX) $(CXXFLAGS) -o ps_thread ps_thread.cpp $(CXXLIBS)


clean:
	rm -f ps_thread
