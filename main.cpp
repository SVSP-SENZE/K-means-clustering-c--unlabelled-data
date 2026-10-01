#include "DataPoint.h"
#include "DataSet.h"
#include "Cluster.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

bool hasCoordinates(const DataPoint& point, const std::vector<double>& expected) {
    const double tolerance = 1e-9;
    if (point.dimension() != expected.size()) {
        return false;
    }
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (!(std::abs(point.getFeatures()[i] - expected[i]) < tolerance)) {
            return false;
        }
    }
    return true;
}

void check(bool condition, const char* description) {
    if (!condition) {
        throw std::runtime_error(description);
    }
    std::cout << "PASS: " << description << '\n';
}

int main() {
    DataPoint first({1.0, 1.0});
    DataPoint second({1.0, 3.0});
    DataPoint third({8.0, 8.0});
    DataPoint fourth({8.0, 10.0});

    DataSet dataset;
    dataset.addPoint(first);
    dataset.addPoint(second);
    dataset.addPoint(third);
    dataset.addPoint(fourth);

    std::cout << "Dataset size: " << dataset.size() << '\n';
    std::cout << "Dataset dimension: " << dataset.dimension() << '\n';
    std::cout << "Squared distance between (1,1) and (1,3): "
              << first.squaredDistanceTo(second) << '\n';

    try {
        dataset.addPoint(DataPoint({1.0, 2.0, 3.0}));
    } catch (const std::invalid_argument& error) {
        std::cout << "Rejected 3D point: " << error.what() << '\n';
    }

    std::cout << "Dataset size after rejected addition: " << dataset.size() << '\n';

    try {
        Cluster cluster(DataPoint({0.0, 0.0}));
        check(cluster.size() == 0 && cluster.getMembers().empty()
                  && hasCoordinates(cluster.getCentroid(), {0.0, 0.0}),
              "Initial centroid (0,0), size 0");

        cluster.addMember(first);
        cluster.addMember(second);
        cluster.updateCentroid();
        check(cluster.size() == 2
                  && hasCoordinates(cluster.getCentroid(), {1.0, 2.0}),
              "2D mean centroid (1,2), size 2");

        bool rejected = false;
        try {
            cluster.addMember(DataPoint({1.0, 2.0, 3.0}));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected && cluster.size() == 2
                  && hasCoordinates(cluster.getCentroid(), {1.0, 2.0})
                  && hasCoordinates(cluster.getMembers()[0], {1.0, 1.0})
                  && hasCoordinates(cluster.getMembers()[1], {1.0, 3.0}),
              "3D member rejected; centroid and members unchanged");

        cluster.clearMembers();
        check(cluster.size() == 0 && cluster.getMembers().empty()
                  && hasCoordinates(cluster.getCentroid(), {1.0, 2.0}),
              "Clear members: size 0, centroid remains (1,2)");

        cluster.updateCentroid();
        check(cluster.size() == 0
                  && hasCoordinates(cluster.getCentroid(), {1.0, 2.0}),
              "Empty update preserves centroid (1,2)");

        Cluster cluster3D(DataPoint({0.0, 0.0, 0.0}));
        cluster3D.addMember(DataPoint({1.0, 2.0, 3.0}));
        cluster3D.addMember(DataPoint({3.0, 4.0, 5.0}));
        cluster3D.updateCentroid();
        check(cluster3D.size() == 2
                  && hasCoordinates(cluster3D.getCentroid(), {2.0, 3.0, 4.0}),
              "3D mean centroid (2,3,4), size 2");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }

    std::cout << "All milestone-2 checks passed.\n";
    return 0;
}
