#pragma once

#include "Cluster.h"
#include "IInitializer.h"

#include <cstddef>
#include <random>
#include <vector>

class KMeans {
public:
    enum class StopReason { NotFitted, ToleranceReached, IterationLimit };

    KMeans(std::size_t k, std::size_t maximumIterations,
           double tolerance, std::mt19937::result_type seed);

    void fit(const DataSet& dataset, const IInitialiser& initialiser);
    const std::vector<Cluster>& getClusters() const;
    std::size_t getIterationCount() const;
    StopReason getStoppingReason() const;
    double getInertia() const;
    std::size_t predict(const DataPoint& point) const;

private:
    std::size_t k;
    std::size_t maximumIterations;
    double tolerance;
    std::mt19937::result_type seed;
    std::vector<Cluster> clusters;
    std::size_t iterationCount = 0;
    StopReason stoppingReason = StopReason::NotFitted;
};
