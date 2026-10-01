#pragma once

#include "DataPoint.h"
#include "DataSet.h"

#include <cstddef>
#include <random>
#include <vector>

class IInitialiser {
public:
    virtual ~IInitialiser() = default;

    virtual std::vector<DataPoint> initialise(
        const DataSet& dataset,
        std::size_t k,
        std::mt19937& rng
    ) const = 0;
};
