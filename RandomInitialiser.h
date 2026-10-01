#pragma once

#include "IInitializer.h"

class RandomInitialiser : public IInitialiser {
public:
    std::vector<DataPoint> initialise(
        const DataSet& dataset,
        std::size_t k,
        std::mt19937& rng
    ) const override;
};
