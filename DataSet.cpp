#include "DataSet.h"

#include <stdexcept>

void DataSet::addPoint(const DataPoint& point) {
    if (!points.empty() && point.dimension() != dimension()) {
        throw std::invalid_argument("Point dimension does not match dataset dimension.");
    }

    points.push_back(point);
}

std::size_t DataSet::size() const {
    return points.size();
}

std::size_t DataSet::dimension() const {
    if (points.empty()) {
        return 0;
    }
    return points.front().dimension();
}

const std::vector<DataPoint>& DataSet::getPoints() const {
    return points;
}
