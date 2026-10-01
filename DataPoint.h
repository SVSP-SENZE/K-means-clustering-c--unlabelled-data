#pragma once

#include <cstddef>
#include <vector>

class DataPoint {
private:
    std::vector<double> features;

public:
    explicit DataPoint(const std::vector<double>& values);

    std::size_t dimension() const;
    const std::vector<double>& getFeatures() const;
    double squaredDistanceTo(const DataPoint& other) const;
};
