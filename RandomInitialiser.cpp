#include "RandomInitialiser.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

std::vector<DataPoint> RandomInitialiser::initialise(
    const DataSet& dataset,
    std::size_t k,
    std::mt19937& rng
) const {
    if (dataset.size() == 0) {
        throw std::invalid_argument("Cannot initialise centres from an empty dataset.");
    }
    if (k == 0 || k > dataset.size()) {
        throw std::invalid_argument("k must be between 1 and the dataset size.");
    }

    std::vector<std::size_t> indices(dataset.size());
    std::iota(indices.begin(), indices.end(), std::size_t{0});
    std::shuffle(indices.begin(), indices.end(), rng);

    std::vector<DataPoint> centres;
    centres.reserve(k);
    for (std::size_t i = 0; i < k; ++i) {
        centres.push_back(dataset.getPoints()[indices[i]]);
    }
    return centres;
}
