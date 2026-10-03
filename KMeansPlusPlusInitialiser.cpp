#include "KMeansPlusPlusInitialiser.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

std::vector<DataPoint> KMeansPlusPlusInitialiser::initialise(
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

    const std::vector<DataPoint>& points = dataset.getPoints();
    std::vector<bool> selected(dataset.size(), false);
    std::vector<DataPoint> centres;
    centres.reserve(k);

    std::uniform_int_distribution<std::size_t> firstChoice(0, dataset.size() - 1);
    std::size_t firstIndex = firstChoice(rng);
    selected[firstIndex] = true;
    centres.push_back(points[firstIndex]);

    while (centres.size() < k) {
        std::vector<std::size_t> unselectedIndices;
        std::vector<std::size_t> weightedIndices;
        std::vector<double> weights;
        double maximumWeight = 0.0;

        for (std::size_t i = 0; i < points.size(); ++i) {
            if (selected[i]) {
                continue;
            }
            unselectedIndices.push_back(i);
            double nearestSquaredDistance = std::numeric_limits<double>::max();
            for (const DataPoint& centre : centres) {
                double distance = points[i].squaredDistanceTo(centre);
                if (!std::isfinite(distance)) {
                    throw std::overflow_error("K-Means++ squared distance is not finite.");
                }
                nearestSquaredDistance = std::min(nearestSquaredDistance, distance);
            }

            // Zero-weight points cannot be chosen while positive weights remain.
            if (nearestSquaredDistance > 0.0) {
                weightedIndices.push_back(i);
                weights.push_back(nearestSquaredDistance);
                maximumWeight = std::max(maximumWeight, nearestSquaredDistance);
            }
        }

        std::size_t nextIndex;
        if (weights.empty()) {
            // All remaining observations coincide with an already selected centre.
            std::uniform_int_distribution<std::size_t> choice(0, unselectedIndices.size() - 1);
            nextIndex = unselectedIndices[choice(rng)];
        } else {
            // Common scaling preserves proportions and avoids overflowing the sum.
            for (double& weight : weights) {
                weight /= maximumWeight;
            }
            std::discrete_distribution<std::size_t> choice(weights.begin(), weights.end());
            nextIndex = weightedIndices[choice(rng)];
        }

        selected[nextIndex] = true;
        centres.push_back(points[nextIndex]);
    }
    return centres;
}
