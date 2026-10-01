#include "DataPoint.h"

#include <cmath>
#include <stdexcept>

DataPoint::DataPoint(const std::vector<double>& values)
    : features(values) {
    if (features.empty()) {
        throw std::invalid_argument("A data point must have at least one feature.");
    }

    for (double value : features) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("Data point features must be finite numbers.");
        }
    }
}

std::size_t DataPoint::dimension() const {
    return features.size();
}

const std::vector<double>& DataPoint::getFeatures() const {
    return features;
}

double DataPoint::squaredDistanceTo(const DataPoint& other) const {
    if (dimension() != other.dimension()) {
        throw std::invalid_argument("Cannot calculate distance between points with different dimensions.");
    }

    double squaredDistance = 0.0;
    for (std::size_t i = 0; i < features.size(); ++i) {
        double difference = features[i] - other.features[i];
        squaredDistance += difference * difference;
    }
    return squaredDistance;
}
