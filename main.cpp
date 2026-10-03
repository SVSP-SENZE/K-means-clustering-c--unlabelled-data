#include "DataPoint.h"
#include "DataSet.h"
#include "Cluster.h"
#include "RandomInitialiser.h"
#include "KMeans.h"

#include <cmath>
#include <iostream>
#include <limits>
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

// Test-only strategy: supplied centres also let us test invalid initializer output.
class FixedInitialiser : public IInitialiser {
private:
    std::vector<DataPoint> centres;

public:
    explicit FixedInitialiser(const std::vector<DataPoint>& centres) : centres(centres) {}

    std::vector<DataPoint> initialise(const DataSet&, std::size_t,
                                      std::mt19937&) const override {
        return centres;
    }
};

bool rejectsConfiguration(std::size_t k, std::size_t iterations, double tolerance) {
    try {
        KMeans model(k, iterations, tolerance, 42);
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

bool rejectsFit(KMeans& model, const DataSet& dataset, const IInitialiser& initialiser) {
    try {
        model.fit(dataset, initialiser);
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

void printResults(const char* title, const KMeans& model) {
    std::cout << title << '\n';
    std::cout << "Iterations: " << model.getIterationCount() << "; stop: ";
    switch (model.getStoppingReason()) {
        case KMeans::StopReason::NotFitted:
            std::cout << "not fitted\n";
            break;
        case KMeans::StopReason::ToleranceReached:
            std::cout << "tolerance reached\n";
            break;
        case KMeans::StopReason::IterationLimit:
            std::cout << "iteration limit\n";
            break;
    }
    for (std::size_t i = 0; i < model.getClusters().size(); ++i) {
        const Cluster& cluster = model.getClusters()[i];
        std::cout << "Cluster " << i << ": centroid (";
        const std::vector<double>& features = cluster.getCentroid().getFeatures();
        for (std::size_t j = 0; j < features.size(); ++j) {
            if (j != 0) {
                std::cout << ',';
            }
            std::cout << features[j];
        }
        std::cout << "), size " << cluster.size() << '\n';
    }
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

    try {
        FixedInitialiser fixed({first, third});
        KMeans model(2, 100, 1e-6, 42);
        check(model.getClusters().empty() && model.getIterationCount() == 0
                  && model.getStoppingReason() == KMeans::StopReason::NotFitted,
              "New KMeans reports not fitted");
        model.fit(dataset, fixed);
        check(model.getClusters().size() == 2
                  && hasCoordinates(model.getClusters()[0].getCentroid(), {1.0, 2.0})
                  && hasCoordinates(model.getClusters()[1].getCentroid(), {8.0, 9.0})
                  && model.getClusters()[0].size() == 2 && model.getClusters()[1].size() == 2
                  && model.getIterationCount() == 2
                  && model.getStoppingReason() == KMeans::StopReason::ToleranceReached,
              "Fixed centres reach (1,2) and (8,9), sizes 2 and 2, in two iterations");
        printResults("Fixed-initializer demo:", model);

        RandomInitialiser randomInitialiser;
        KMeans randomModel(2, 100, 1e-6, 42);
        randomModel.fit(dataset, randomInitialiser);
        const std::vector<Cluster> firstRun = randomModel.getClusters();
        const std::size_t firstCount = randomModel.getIterationCount();
        randomModel.fit(dataset, randomInitialiser);
        bool repeated = randomModel.getClusters().size() == firstRun.size()
            && randomModel.getIterationCount() == firstCount;
        for (std::size_t i = 0; repeated && i < firstRun.size(); ++i) {
            repeated = hasCoordinates(randomModel.getClusters()[i].getCentroid(),
                                      firstRun[i].getCentroid().getFeatures())
                && randomModel.getClusters()[i].size() == firstRun[i].size();
            for (std::size_t j = 0; repeated && j < firstRun[i].size(); ++j) {
                repeated = hasCoordinates(randomModel.getClusters()[i].getMembers()[j],
                                          firstRun[i].getMembers()[j].getFeatures());
            }
        }
        check(repeated, "Repeated random fits restart the seeded generator and replace results");
        printResults("Random-initializer demo (seed 42):", randomModel);

        KMeans boundary(2, 1, 1.0, 42);
        boundary.fit(dataset, fixed);
        check(boundary.getIterationCount() == 1
                  && boundary.getStoppingReason() == KMeans::StopReason::ToleranceReached,
              "Movement equal to tolerance stops, including at the iteration limit");

        DataSet fractional;
        fractional.addPoint(DataPoint({0.75}));
        FixedInitialiser zero({DataPoint({0.0})});
        KMeans fractionalModel(1, 10, 0.6, 42);
        fractionalModel.fit(fractional, zero);
        check(fractionalModel.getIterationCount() == 2
                  && hasCoordinates(fractionalModel.getClusters()[0].getCentroid(), {0.75}),
              "Euclidean movement 0.75 exceeds tolerance 0.6");

        FixedInitialiser duplicateStarts({first, first});
        KMeans ties(2, 1, 0.0, 42);
        ties.fit(dataset, duplicateStarts);
        check(ties.getClusters()[0].size() == 4 && ties.getClusters()[1].size() == 0
                  && hasCoordinates(ties.getClusters()[0].getCentroid(), {4.5, 5.5})
                  && hasCoordinates(ties.getClusters()[1].getCentroid(), {1.0, 1.0}),
              "Ties choose index 0; empty cluster preserves its centroid");

        // Point 2 would change clusters if an extra reassignment were performed.
        DataSet earlyData;
        for (double value : {0.0, 2.0, 3.0, 10.0}) {
            earlyData.addPoint(DataPoint({value}));
        }
        FixedInitialiser earlyStarts({DataPoint({0.0}), DataPoint({2.0})});
        KMeans limited(2, 1, 0.0, 42);
        limited.fit(earlyData, earlyStarts);
        check(limited.getIterationCount() == 1
                  && limited.getStoppingReason() == KMeans::StopReason::IterationLimit
                  && limited.getClusters()[0].size() == 1 && limited.getClusters()[1].size() == 3
                  && hasCoordinates(limited.getClusters()[0].getCentroid(), {0.0})
                  && hasCoordinates(limited.getClusters()[1].getCentroid(), {5.0})
                  && hasCoordinates(limited.getClusters()[1].getMembers()[0], {2.0}),
              "Iteration-limit stop preserves the completed assignment/update pair");
        KMeans looseTolerance(2, 10, 3.0, 42);
        looseTolerance.fit(earlyData, earlyStarts);
        check(looseTolerance.getIterationCount() == 1
                  && looseTolerance.getStoppingReason() == KMeans::StopReason::ToleranceReached
                  && looseTolerance.getClusters()[0].size() == 1
                  && looseTolerance.getClusters()[1].size() == 3
                  && hasCoordinates(looseTolerance.getClusters()[1].getCentroid(), {5.0}),
              "Tolerance stop also preserves members without final reassignment");

        DataSet data3D;
        data3D.addPoint(DataPoint({1.0, 2.0, 3.0}));
        data3D.addPoint(DataPoint({3.0, 4.0, 5.0}));
        KMeans model3D(1, 10, 0.0, 42);
        model3D.fit(data3D, randomInitialiser);
        check(hasCoordinates(model3D.getClusters()[0].getCentroid(), {2.0, 3.0, 4.0})
                  && model3D.getClusters()[0].size() == 2 && model3D.getIterationCount() == 2,
              "3D fit with zero tolerance reaches the mean");

        check(rejectsConfiguration(0, 10, 0.0) && rejectsConfiguration(2, 0, 0.0)
                  && rejectsConfiguration(2, 10, -1.0)
                  && rejectsConfiguration(2, 10, std::numeric_limits<double>::infinity())
                  && rejectsConfiguration(2, 10, std::numeric_limits<double>::quiet_NaN()),
              "Invalid k, iteration count, and tolerances are rejected");
        DataSet empty;
        KMeans tooMany(5, 10, 0.0, 42);
        check(rejectsFit(model, empty, fixed) && rejectsFit(tooMany, dataset, fixed),
              "Empty dataset and k greater than dataset size are rejected");
        FixedInitialiser tooFew({first});
        FixedInitialiser tooManyCentres({first, second, third});
        FixedInitialiser wrongDimension({first, DataPoint({1.0, 2.0, 3.0})});
        check(rejectsFit(model, dataset, tooFew) && rejectsFit(model, dataset, tooManyCentres)
                  && rejectsFit(model, dataset, wrongDimension),
              "Initializer output count and dimensions are validated");
        check(model.getIterationCount() == 2
                  && model.getStoppingReason() == KMeans::StopReason::ToleranceReached
                  && model.getClusters()[0].size() == 2 && model.getClusters()[1].size() == 2
                  && hasCoordinates(model.getClusters()[0].getCentroid(), {1.0, 2.0})
                  && hasCoordinates(model.getClusters()[1].getCentroid(), {8.0, 9.0}),
              "Rejected fits preserve the previous completed results");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }

    std::cout << "All milestone-4 checks passed.\n";
    return 0;
}
