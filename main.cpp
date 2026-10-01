#include "DataPoint.h"
#include "DataSet.h"
#include "Cluster.h"
#include "RandomInitialiser.h"

#include <cmath>
#include <iostream>
#include <random>
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

bool allFromDataset(const std::vector<DataPoint>& centres, const DataSet& dataset) {
    for (const DataPoint& centre : centres) {
        bool found = false;
        for (const DataPoint& point : dataset.getPoints()) {
            if (hasCoordinates(centre, point.getFeatures())) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

bool haveDistinctCoordinates(const std::vector<DataPoint>& points) {
    for (std::size_t i = 0; i < points.size(); ++i) {
        for (std::size_t j = i + 1; j < points.size(); ++j) {
            if (hasCoordinates(points[i], points[j].getFeatures())) {
                return false;
            }
        }
    }
    return true;
}

bool rejectsInitialisation(const IInitialiser& initialiser,
                           const DataSet& dataset, std::size_t k,
                           std::mt19937& rng) {
    try {
        initialiser.initialise(dataset, k, rng);
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
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

    try {
        RandomInitialiser randomInitialiser;
        const IInitialiser& initialiser = randomInitialiser;
        std::mt19937 rng(42);
        const std::size_t k = 2;
        const std::vector<DataPoint> centres = initialiser.initialise(dataset, k, rng);
        check(centres.size() == k && allFromDataset(centres, dataset),
              "Base-reference call returns k input points");
        check(haveDistinctCoordinates(centres),
              "Unique input coordinates are selected without repetition");

        std::mt19937 firstRng(12345);
        std::mt19937 secondRng(12345);
        const std::vector<DataPoint> firstCentres =
            initialiser.initialise(dataset, k, firstRng);
        const std::vector<DataPoint> secondCentres =
            initialiser.initialise(dataset, k, secondRng);
        bool sameOrder = firstCentres.size() == k && secondCentres.size() == k;
        for (std::size_t i = 0; sameOrder && i < k; ++i) {
            sameOrder = hasCoordinates(firstCentres[i], secondCentres[i].getFeatures());
        }
        check(sameOrder, "Fresh generators with the same seed give the same ordered centres");

        const std::vector<DataPoint> single = initialiser.initialise(dataset, 1, rng);
        check(single.size() == 1 && allFromDataset(single, dataset),
              "k=1 returns one input point");
        const std::vector<DataPoint> all =
            initialiser.initialise(dataset, dataset.size(), rng);
        check(all.size() == dataset.size() && allFromDataset(all, dataset)
                  && haveDistinctCoordinates(all),
              "k=dataset.size() returns every input point once");

        DataSet emptyDataset;
        check(rejectsInitialisation(initialiser, emptyDataset, 1, rng),
              "Empty dataset is rejected");
        check(rejectsInitialisation(initialiser, dataset, 0, rng),
              "k=0 is rejected");
        check(rejectsInitialisation(initialiser, dataset, dataset.size() + 1, rng),
              "k larger than dataset size is rejected");

        DataSet duplicates;
        duplicates.addPoint(DataPoint({5.0, 5.0}));
        duplicates.addPoint(DataPoint({5.0, 5.0}));
        duplicates.addPoint(DataPoint({5.0, 5.0}));
        const std::vector<DataPoint> duplicateCentres =
            initialiser.initialise(duplicates, duplicates.size(), rng);
        check(duplicateCentres.size() == duplicates.size()
                  && allFromDataset(duplicateCentres, duplicates),
              "Identical coordinates are accepted for valid k");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }

    std::cout << "All milestone-3 checks passed.\n";
    return 0;
}
