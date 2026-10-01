#include "DataPoint.h"
#include "DataSet.h"

#include <iostream>
#include <stdexcept>

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
    return 0;
}
