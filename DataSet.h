#pragma once

#include "DataPoint.h"

#include <cstddef>
#include <vector>

class DataSet {
private:
    std::vector<DataPoint> points;

public:
    void addPoint(const DataPoint& point);
    std::size_t size() const;
    std::size_t dimension() const;
    const std::vector<DataPoint>& getPoints() const;
};
