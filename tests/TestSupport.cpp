#include "DataPoint.h"
#include "DataSet.h"
#include "Cluster.h"
#include "RandomInitialiser.h"
#include "KMeans.h"
#include "KMeansPlusPlusInitialiser.h"
#include "TestSupport.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
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

// Match each returned centre to one unused observation, including duplicates.
bool respectsInputMultiplicities(const std::vector<DataPoint>& centres, const DataSet& dataset) {
    std::vector<bool> matched(dataset.size(), false);
    for (const DataPoint& centre : centres) {
        bool found = false;
        for (std::size_t i = 0; i < dataset.size(); ++i) {
            if (!matched[i] && hasCoordinates(centre, dataset.getPoints()[i].getFeatures())) {
                matched[i] = true;
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

bool hasExpectedDemoClusters(const KMeans& model) {
    const std::vector<Cluster>& clusters = model.getClusters();
    if (clusters.size() != 2 || clusters[0].size() != 2 || clusters[1].size() != 2) {
        return false;
    }
    return (hasCoordinates(clusters[0].getCentroid(), {1.0, 2.0})
            && hasCoordinates(clusters[1].getCentroid(), {8.0, 9.0}))
        || (hasCoordinates(clusters[1].getCentroid(), {1.0, 2.0})
            && hasCoordinates(clusters[0].getCentroid(), {8.0, 9.0}));
}

bool sameFittedResults(const KMeans& first, const KMeans& second) {
    if (first.getIterationCount() != second.getIterationCount()
        || first.getStoppingReason() != second.getStoppingReason()
        || first.getClusters().size() != second.getClusters().size()
        || !(std::abs(first.getInertia() - second.getInertia()) < 1e-9)) {
        return false;
    }
    for (std::size_t i = 0; i < first.getClusters().size(); ++i) {
        const Cluster& left = first.getClusters()[i];
        const Cluster& right = second.getClusters()[i];
        if (left.size() != right.size()
            || !hasCoordinates(left.getCentroid(), right.getCentroid().getFeatures())) {
            return false;
        }
        for (std::size_t j = 0; j < left.size(); ++j) {
            if (!hasCoordinates(left.getMembers()[j], right.getMembers()[j].getFeatures())) {
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

