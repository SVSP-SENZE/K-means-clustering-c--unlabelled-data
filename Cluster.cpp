#include "Cluster.h"

#include <stdexcept>

Cluster::Cluster(const DataPoint& initialCentroid)
    : centroid(initialCentroid) {
}

void Cluster::addMember(const DataPoint& point) {
    if (point.dimension() != centroid.dimension()) {
        throw std::invalid_argument("Member dimension does not match centroid dimension.");
    }
    members.push_back(point);
}

void Cluster::clearMembers() {
    members.clear();
}

void Cluster::updateCentroid() {
    if (members.empty()) {
        return;
    }

    std::vector<double> mean(centroid.dimension(), 0.0);
    for (const DataPoint& point : members) {
        const std::vector<double>& features = point.getFeatures();
        for (std::size_t i = 0; i < mean.size(); ++i) {
            mean[i] += features[i];
        }
    }

    for (double& coordinate : mean) {
        coordinate /= static_cast<double>(members.size());
    }

    // Validate the new point before replacing the current centroid.
    DataPoint replacement(mean);
    centroid = replacement;
}

std::size_t Cluster::size() const {
    return members.size();
}

const DataPoint& Cluster::getCentroid() const {
    return centroid;
}

const std::vector<DataPoint>& Cluster::getMembers() const {
    return members;
}
