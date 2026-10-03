#include "TestSupport.h"
#include "CsvIO.h"
#include "RandomInitialiser.h"
#include "KMeansPlusPlusInitialiser.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(double actual, double expected) {
    return std::isfinite(actual) && std::isfinite(expected)
        && std::abs(actual - expected) <= 1e-9 * std::max(1.0, std::abs(expected));
}

// A tiny reusable exception check; unexpected exception types still fail the test.
template <typename Exception, typename Action>
void expectThrows(Action action, const std::string& messagePart) {
    try {
        action();
    } catch (const Exception& error) {
        require(std::string(error.what()).find(messagePart) != std::string::npos,
                "Exception message does not contain: " + messagePart);
        return;
    }
    throw std::runtime_error("Expected exception: " + messagePart);
}

DataSet makeData(const std::vector<std::vector<double>>& rows) {
    DataSet data;
    for (const std::vector<double>& row : rows) {
        data.addPoint(DataPoint(row));
    }
    return data;
}

// Members are unchanged copies of input points, so exact equality is appropriate
// for observation identity. Numerical means/distances use a tolerance instead.
bool isPartition(const DataSet& data, const std::vector<Cluster>& clusters) {
    std::vector<bool> used(data.size(), false);
    std::size_t total = 0;
    for (const Cluster& cluster : clusters) {
        total += cluster.size();
        for (const DataPoint& member : cluster.getMembers()) {
            bool found = false;
            for (std::size_t i = 0; i < data.size(); ++i) {
                if (!used[i] && member.getFeatures() == data.getPoints()[i].getFeatures()) {
                    used[i] = true;
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
    }
    return total == data.size() && std::all_of(used.begin(), used.end(), [](bool value) { return value; });
}

void verifyResults(const DataSet& data, const KMeans& model, std::size_t k) {
    require(model.getClusters().size() == k, "Cluster count must equal k");
    require(isPartition(data, model.getClusters()), "Every input occurrence must be assigned exactly once");
    std::size_t total = 0;
    long double inertia = 0.0;
    for (const Cluster& cluster : model.getClusters()) {
        total += cluster.size();
        require(cluster.getCentroid().dimension() == data.dimension(), "Centroid dimension");
        for (std::size_t j = 0; j < data.dimension(); ++j) {
            long double sum = 0.0;
            for (const DataPoint& member : cluster.getMembers()) {
                sum += member.getFeatures()[j];
                long double delta = static_cast<long double>(member.getFeatures()[j])
                    - cluster.getCentroid().getFeatures()[j];
                inertia += delta * delta;
            }
            if (cluster.size() != 0) {
                require(near(cluster.getCentroid().getFeatures()[j],
                             static_cast<double>(sum / cluster.size())), "Centroid must be its members' mean");
            }
        }
    }
    require(total == data.size(), "Cluster sizes must sum to dataset size");
    require(near(model.getInertia(), static_cast<double>(inertia)), "Inertia must describe reported members");
}

void testDataPoint() {
    expectThrows<std::invalid_argument>([] { DataPoint point({}); }, "at least one feature");
    for (double value : {std::numeric_limits<double>::quiet_NaN(),
                         std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity()}) {
        expectThrows<std::invalid_argument>([value] { DataPoint point({1.0, value}); }, "finite");
    }
    std::vector<double> values{1.0, 2.0, 3.0};
    DataPoint point(values);
    values[0] = 100.0;
    require(hasCoordinates(point, {1.0, 2.0, 3.0}), "Point owns an independent feature copy");
    require(point.dimension() == 3, "Point dimension");
    const DataPoint other({4.0, 6.0, 3.0});
    require(near(point.squaredDistanceTo(other), 25.0), "3D squared distance should be 25");
    require(near(other.squaredDistanceTo(point), 25.0), "Distance symmetry");
    require(near(point.squaredDistanceTo(point), 0.0), "Self distance");
    require(near(DataPoint({-2.0}).squaredDistanceTo(DataPoint({3.0})), 25.0), "1D distance");
    require(near(DataPoint({0.0, 0.0}).squaredDistanceTo(DataPoint({3.0, 4.0})), 25.0), "2D distance");
    expectThrows<std::invalid_argument>([&] { point.squaredDistanceTo(DataPoint({1.0})); }, "different dimensions");
    expectThrows<std::invalid_argument>([&] { DataPoint({1.0}).squaredDistanceTo(point); }, "different dimensions");
}

void testDataSet() {
    DataSet data;
    require(data.size() == 0 && data.dimension() == 0 && data.getPoints().empty(), "Empty dataset state");
    data.addPoint(DataPoint({1.0, 2.0}));
    expectThrows<std::invalid_argument>([&] { data.addPoint(DataPoint({3.0})); }, "dimension");
    expectThrows<std::invalid_argument>([&] { data.addPoint(DataPoint({3.0, 4.0, 5.0})); }, "dimension");
    require(data.size() == 1 && data.dimension() == 2
            && hasCoordinates(data.getPoints()[0], {1.0, 2.0}), "Rejected additions must not modify data");
    data.addPoint(DataPoint({3.0, 4.0}));
    require(data.size() == 2 && hasCoordinates(data.getPoints()[1], {3.0, 4.0}), "Valid addition after rejection");
}

void testCluster() {
    Cluster cluster(DataPoint({9.0, 8.0, 7.0}));
    cluster.updateCentroid();
    require(cluster.size() == 0 && hasCoordinates(cluster.getCentroid(), {9.0, 8.0, 7.0}), "Initial empty policy");
    cluster.addMember(DataPoint({0.0, 0.0, 0.0}));
    cluster.addMember(DataPoint({1.0, 2.0, 3.0}));
    cluster.addMember(DataPoint({1.0, 1.0, 1.0}));
    cluster.updateCentroid();
    require(hasCoordinates(cluster.getCentroid(), {2.0 / 3.0, 1.0, 4.0 / 3.0}), "Fractional 3D mean");
    expectThrows<std::invalid_argument>([&] { cluster.addMember(DataPoint({1.0})); }, "dimension");
    require(cluster.size() == 3 && hasCoordinates(cluster.getCentroid(), {2.0 / 3.0, 1.0, 4.0 / 3.0}),
            "Rejected member must preserve state");
    cluster.clearMembers();
    cluster.updateCentroid();
    require(cluster.size() == 0 && hasCoordinates(cluster.getCentroid(), {2.0 / 3.0, 1.0, 4.0 / 3.0}),
            "Clearing and empty update preserve the last centroid");
}

void testInitializers() {
    RandomInitialiser random;
    KMeansPlusPlusInitialiser plusPlus;
    const DataSet duplicates = makeData({{1.0, 1.0}, {1.0, 1.0}, {8.0, 8.0}, {8.0, 8.0}, {8.0, 8.0}});
    const DataSet identical = makeData({{5.0}, {5.0}, {5.0}});
    const DataSet empty;
    for (const IInitialiser* initialiser : std::vector<const IInitialiser*>{&random, &plusPlus}) {
        std::mt19937 rng(42);
        const std::mt19937 before = rng;
        expectThrows<std::invalid_argument>([&] { initialiser->initialise(empty, 1, rng); }, "empty");
        expectThrows<std::invalid_argument>([&] { initialiser->initialise(duplicates, 0, rng); }, "k must");
        expectThrows<std::invalid_argument>([&] { initialiser->initialise(duplicates, 6, rng); }, "k must");
        require(rng == before, "Invalid initialization must not consume randomness");
        for (const DataSet* data : {&duplicates, &identical}) {
            for (std::size_t k = 1; k <= data->size(); ++k) {
                std::vector<DataPoint> selected = initialiser->initialise(*data, k, rng);
                require(selected.size() == k && respectsInputMultiplicities(selected, *data),
                        "Initializer must preserve observation multiplicities");
            }
        }
        KMeans model(1, 10, 0.0, 42);
        expectThrows<std::invalid_argument>([&] { model.fit(empty, *initialiser); }, "nonempty");
        KMeans excessive(6, 10, 0.0, 42);
        expectThrows<std::invalid_argument>([&] { excessive.fit(duplicates, *initialiser); }, "at least k");
    }
    expectThrows<std::invalid_argument>([] { KMeans model(0, 10, 0.0, 42); }, "positive");
}

void testFixedCentresAndLabels() {
    const DataSet data = makeData({{1.0, 1.0}, {1.0, 3.0}, {8.0, 8.0}, {8.0, 10.0}});
    FixedInitialiser forward({DataPoint({1.0, 1.0}), DataPoint({8.0, 8.0})});
    FixedInitialiser reversed({DataPoint({8.0, 8.0}), DataPoint({1.0, 1.0})});
    for (const IInitialiser* initialiser : std::vector<const IInitialiser*>{&forward, &reversed}) {
        KMeans model(2, 100, 1e-6, 42);
        model.fit(data, *initialiser);
        require(hasExpectedDemoClusters(model), "Expected centres and sizes regardless of label order");
        require(near(model.getInertia(), 4.0), "Hand-calculated inertia");
        verifyResults(data, model, 2);
        const KMeans snapshot = model;
        std::size_t nearby = model.predict(DataPoint({1.0, 2.1}));
        require(hasCoordinates(model.getClusters()[nearby].getCentroid(), {1.0, 2.0}),
                "Prediction follows centroid coordinates, not an assumed label");
        require(sameFittedResults(model, snapshot), "Prediction leaves model unchanged");
        model.fit(data, *initialiser);
        require(sameFittedResults(model, snapshot), "Repeated fitting replaces results");
        verifyResults(data, model, 2);
    }
    KMeans single(1, 100, 0.0, 42);
    RandomInitialiser random;
    single.fit(data, random);
    require(hasCoordinates(single.getClusters()[0].getCentroid(), {4.5, 5.5})
            && near(single.getInertia(), 102.0), "k=1 overall mean and inertia");
    verifyResults(data, single, 1);
}

void testTiesAndIterationLimit() {
    const DataSet ties = makeData({{-1.0}, {0.0}, {1.0}});
    FixedInitialiser starts({DataPoint({-1.0}), DataPoint({1.0})});
    KMeans tied(2, 1, 0.0, 42);
    tied.fit(ties, starts);
    require(tied.getClusters()[0].size() == 2 && tied.getClusters()[1].size() == 1,
            "Equidistant middle observation goes to index 0");
    require(tied.predict(DataPoint({0.25})) == 0, "Prediction tie goes to index 0");
    verifyResults(ties, tied, 2);
    const DataSet early = makeData({{0.0}, {2.0}, {3.0}, {10.0}});
    FixedInitialiser earlyStarts({DataPoint({0.0}), DataPoint({2.0})});
    KMeans limited(2, 1, 0.0, 42);
    limited.fit(early, earlyStarts);
    require(limited.getIterationCount() == 1
            && limited.getStoppingReason() == KMeans::StopReason::IterationLimit, "Maximum iterations respected");
    require(near(limited.getInertia(), 38.0) && limited.predict(DataPoint({2.0})) == 0
            && limited.getClusters()[1].size() == 3, "Early stop has no hidden reassignment");
    verifyResults(early, limited, 2);
}

void testPartitions() {
    // This intentionally incorrect partition has the right total size, but loses
    // one distinct, very close input entry. The accounting check must reject it.
    const DataSet close = makeData({{0.0}, {1e-12}, {1.0}, {1.0}});
    Cluster wrong(DataPoint({0.5}));
    for (double value : {0.0, 0.0, 1.0, 1.0}) {
        wrong.addMember(DataPoint({value}));
    }
    require(!isPartition(close, {wrong}), "Accounting must distinguish close values and duplicate counts");

    const std::vector<DataSet> datasets{
        makeData({{1.0, 1.0}, {1.0, 3.0}, {8.0, 8.0}, {8.0, 10.0}}),
        makeData({{1.0, 1.0}, {1.0, 1.0}, {8.0, 8.0}, {8.0, 8.0}, {8.0, 10.0}}),
        makeData({{5.0}, {5.0}, {5.0}, {5.0}}),
        makeData({{1.0, 2.0, 3.0}, {3.0, 4.0, 5.0}, {9.0, 8.0, 7.0}}),
        close
    };
    RandomInitialiser random;
    KMeansPlusPlusInitialiser plusPlus;
    std::size_t fits = 0;
    for (const DataSet& data : datasets) {
        for (const IInitialiser* initialiser : std::vector<const IInitialiser*>{&random, &plusPlus}) {
            for (unsigned int seed : {0u, 1u, 42u}) {
                for (std::size_t k = 1; k <= data.size(); ++k) {
                    for (std::size_t limit : {std::size_t{1}, std::size_t{30}}) {
                        KMeans model(k, limit, 1e-9, seed);
                        model.fit(data, *initialiser);
                        verifyResults(data, model, k);
                        require(model.getIterationCount() >= 1 && model.getIterationCount() <= limit,
                                "Iteration bounds");
                        ++fits;
                    }
                }
            }
        }
    }
    require(fits == 240, "Expected 240 fitted partitions");
    std::cout << "Verified 240 fitted partitions: occurrence counts, total sizes, means, and inertia.\n";
}

void testCsv() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path root = std::filesystem::temp_directory_path()
        / ("minicluster-cpp-tests-" + std::to_string(stamp));
    require(std::filesystem::create_directory(root), "Create a private CSV fixture directory");
    const std::filesystem::path input = root / "input.csv";
    // Only these two exact paths are removed; cleanup is non-recursive.
    try {
        const auto write = [&](const std::string& text) {
            std::ofstream output(input, std::ios::binary);
            output << text;
            output.close();
            require(static_cast<bool>(output), "Write CSV fixture");
        };
        write("\r\n \t\r\n+1e0, .5, -2.\r\n3,1.5,4");
        DataSet data = csv::readDataSet(input);
        require(data.size() == 2 && data.dimension() == 3
                && hasCoordinates(data.getPoints()[0], {1.0, 0.5, -2.0})
                && hasCoordinates(data.getPoints()[1], {3.0, 1.5, 4.0}), "CSV valid syntax and blank lines");
        const std::vector<std::pair<std::string, std::string>> invalid{
            {"", "no data points"}, {"\n \t\r\n", "no data points"},
            {"x,y\n", "line 1, field 1"}, {"1,\n", "field 2: empty field"},
            {",2\n", "field 1: empty field"}, {"1,,2\n", "field 2: empty field"},
            {"1, \t\n", "field 2: empty field"}, {"1,2\n3\n", "line 2: expected 2 fields, got 1"},
            {"\n1,2\n3,4,5\n", "line 3: expected 2 fields, got 3"},
            {"1,2oops\n", "field 2"}, {"1,NaN\n", "finite decimal"},
            {"1,inf\n", "finite decimal"}, {"1,-Infinity\n", "finite decimal"},
            {"1,1e9999\n", "outside the double range"}, {"1,1e-9999\n", "outside the double range"},
            {"\"1\",2\n", "field 1"}, {"1;2\n", "field 1"},
            {"0x1p2,2\n", "field 1"}, {"+-1,2\n", "invalid number"},
            {"1 2,3\n", "field 1"}, {"1,2 #comment\n", "field 2"},
            {"\xEF\xBB\xBF" "1,2\n", "field 1"}
        };
        for (const auto& example : invalid) {
            write(example.first);
            expectThrows<std::invalid_argument>([&] { csv::readDataSet(input); }, example.second);
        }
        expectThrows<std::runtime_error>([&] { csv::readDataSet(root / "missing.csv"); }, "missing");
        expectThrows<std::runtime_error>([&] { csv::readDataSet(root); }, "regular file");
    } catch (...) {
        std::filesystem::remove(input);
        std::filesystem::remove(root);
        throw;
    }
    std::filesystem::remove(input);
    std::filesystem::remove(root);
}
}

int main() {
    std::size_t failures = 0;
    try {
        if (runRegressionChecks() != 0) {
            ++failures;
        }
    } catch (const std::exception& error) {
        std::cerr << "FAIL: regression checks: " << error.what() << '\n';
        ++failures;
    }
    const std::vector<std::pair<const char*, void (*)()>> groups{
        {"DataPoint validation, ownership, and squared distances", testDataPoint},
        {"DataSet dimensions and unchanged state after rejection", testDataSet},
        {"Cluster fractional means and empty-cluster policy", testCluster},
        {"Both initializers: invalid input and duplicate observations", testInitializers},
        {"Fixed-centre results independent of label order, k=1, refit, and prediction", testFixedCentresAndLabels},
        {"Lowest-index ties and maximum-iteration termination", testTiesAndIterationLimit},
        {"Duplicate-aware membership and centroid invariants", testPartitions},
        {"CSV validation in the C++ test executable", testCsv}
    };
    for (const auto& group : groups) {
        try {
            group.second();
            std::cout << "PASS GROUP: " << group.first << '\n';
        } catch (const std::exception& error) {
            std::cerr << "FAIL GROUP: " << group.first << ": " << error.what() << '\n';
            ++failures;
        }
    }
    if (failures != 0) {
        std::cerr << failures << " test group(s) failed.\n";
        return 1;
    }
    std::cout << "All C++ tests passed: 54 regression checks and 8 additional groups.\n";
    return 0;
}
