#pragma once

#include "DataPoint.h"

#include <cstddef>
#include <vector>

class Cluster {
private:
    DataPoint centroid;
    std::vector<DataPoint> members;

public:
    explicit Cluster(const DataPoint& initialCentroid);

    void addMember(const DataPoint& point);
    void clearMembers();
    void updateCentroid();
    std::size_t size() const;
    const DataPoint& getCentroid() const;
    const std::vector<DataPoint>& getMembers() const;
};
