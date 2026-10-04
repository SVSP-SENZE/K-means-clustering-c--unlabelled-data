CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic
APP = MiniCluster.exe
TESTS = MiniClusterTests.exe

APP_SOURCES = DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp \
	KMeansPlusPlusInitialiser.cpp KMeans.cpp CsvIO.cpp JsonIO.cpp SvgPlot.cpp \
	CommandLine.cpp main.cpp
TEST_SOURCES = DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp \
	KMeansPlusPlusInitialiser.cpp KMeans.cpp CsvIO.cpp tests/TestSupport.cpp \
	tests/RegressionChecks.cpp tests/TestMain.cpp

.PHONY: all run tests check clean

all: $(APP)

$(APP): $(APP_SOURCES) $(wildcard *.h) include/nlohmann/json.hpp
	$(CXX) $(CXXFLAGS) -Iinclude $(APP_SOURCES) -o $@

$(TESTS): $(TEST_SOURCES) $(wildcard *.h) tests/TestSupport.h
	$(CXX) $(CXXFLAGS) -I. $(TEST_SOURCES) -o $@

run: $(APP)
	./$(APP)

tests: $(TESTS)
	./$(TESTS)

check: tests $(APP)
	python test_csv_cli.py
	python test_menu.py

clean:
	$(RM) $(APP) $(TESTS)
