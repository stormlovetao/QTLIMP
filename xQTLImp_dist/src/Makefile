# Project: xQTLImp

CPP      = g++
CC       = gcc
OBJ      = xQTLImp_dist.o utils.o impute.o load_info.o comp.o
LINKOBJ  =  xQTLImp_dist.o utils.o impute.o load_info.o comp.o
BIN      =  xQTLImp_dist

$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -o $(BIN) $(LIBS) -lz -pthread 

utils.o: utils.cpp
	$(CPP) -c utils.cpp -o utils.o $(CXXFLAGS) -std=c++11

impute.o: impute.cpp
	$(CPP) -c impute.cpp -o impute.o $(CXXFLAGS) -std=c++11


xQTLImp_dist.o: xQTLImp_dist.cpp
	$(CPP) -c xQTLImp_dist.cpp -o xQTLImp_dist.o $(CXXFLAGS) -std=c++11

load_info.o: load_info.cpp
	$(CPP) -c load_info.cpp -o load_info.o $(CXXFLAGS) -std=c++11
comp.o: comp.cpp
	$(CPP) -c comp.cpp -o comp.o $(CXXFLAGS) -std=c++11


clean:
	@echo "Removing objects..."
	rm -f *.o
	rm -f $(BIN)






