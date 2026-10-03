#pragma once

#include "KMeans.h"

#include <string>
#include <vector>

bool hasCoordinates(const DataPoint& point, const std::vector<double>& expected);
void check(bool condition, const char* description);
bool allFromDataset(const std::vector<DataPoint>& centres, const DataSet& dataset);
bool haveDistinctCoordinates(const std::vector<DataPoint>& points);
bool respectsInputMultiplicities(const std::vector<DataPoint>& centres, const DataSet& dataset);
bool hasExpectedDemoClusters(const KMeans& model);
bool sameFittedResults(const KMeans& first, const KMeans& second);
bool rejectsInitialisation(const IInitialiser&, const DataSet&, std::size_t, std::mt19937&);
bool rejectsConfiguration(std::size_t k, std::size_t iterations, double tolerance);
bool rejectsFit(KMeans&, const DataSet&, const IInitialiser&);
void printResults(const char* title, const KMeans& model);
int runRegressionChecks();

// Test-only strategy: supplied centres also let us test invalid initializer output.
class FixedInitialiser : public IInitialiser {
private:
    std::vector<DataPoint> centres;

public:
    explicit FixedInitialiser(const std::vector<DataPoint>& centres) : centres(centres) {}

    std::vector<DataPoint> initialise(const DataSet&, std::size_t,
                                      std::mt19937&) const override {
        return centres;
    }
};

