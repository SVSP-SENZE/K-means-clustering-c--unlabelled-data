#include "KMeans.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

KMeans::KMeans(std::size_t k, std::size_t maximumIterations,
               double tolerance, std::mt19937::result_type seed)
    : k(k), maximumIterations(maximumIterations), tolerance(tolerance), seed(seed) {
    if (k == 0 || maximumIterations == 0) {
        throw std::invalid_argument("k and maximum iterations must be positive.");
    }
    if (!std::isfinite(tolerance) || tolerance < 0.0) {
        throw std::invalid_argument("Tolerance must be finite and nonnegative.");
    }
}

void KMeans::fit(const DataSet& dataset, const IInitialiser& initialiser) {
    if (dataset.size() == 0 || k > dataset.size()) {
        throw std::invalid_argument("Dataset must be nonempty and contain at least k points.");
    }

    std::mt19937 rng(seed);
    const std::vector<DataPoint> initialCentroids = initialiser.initialise(dataset, k, rng);
    if (initialCentroids.size() != k) {
        throw std::invalid_argument("Initializer must return exactly k centroids.");
    }
    for (const DataPoint& centroid : initialCentroids) {
        if (centroid.dimension() != dataset.dimension()) {
            throw std::invalid_argument("Initializer centroid dimension must match the dataset.");
        }
        for (double value : centroid.getFeatures()) {
            if (!std::isfinite(value)) {
                throw std::invalid_argument("Initializer centroids must be finite.");
            }
        }
    }

    // Work locally so a failed fit does not replace previously completed results.
    std::vector<Cluster> workingClusters;
    workingClusters.reserve(k);
    for (const DataPoint& centroid : initialCentroids) {
        workingClusters.emplace_back(centroid);
    }

    std::size_t completedIterations = 0;
    StopReason reason = StopReason::IterationLimit;
    while (completedIterations < maximumIterations) {
        for (Cluster& cluster : workingClusters) {
            cluster.clearMembers();
        }

        for (const DataPoint& point : dataset.getPoints()) {
            std::size_t nearest = 0;
            double nearestDistance = 0.0;
            for (std::size_t i = 0; i < workingClusters.size(); ++i) {
                double distance = point.squaredDistanceTo(workingClusters[i].getCentroid());
                if (!std::isfinite(distance)) {
                    throw std::overflow_error("Squared distance is not finite.");
                }
                // Strictly less preserves the lowest index when distances tie.
                if (i == 0 || distance < nearestDistance) {
                    nearest = i;
                    nearestDistance = distance;
                }
            }
            workingClusters[nearest].addMember(point);
        }

        double maximumMovement = 0.0;
        for (Cluster& cluster : workingClusters) {
            DataPoint previousCentroid = cluster.getCentroid();
            cluster.updateCentroid();
            double squaredMovement = previousCentroid.squaredDistanceTo(cluster.getCentroid());
            if (!std::isfinite(squaredMovement)) {
                throw std::overflow_error("Squared centroid movement is not finite.");
            }
            maximumMovement = std::max(maximumMovement, std::sqrt(squaredMovement));
        }

        ++completedIterations;
        if (maximumMovement <= tolerance) {
            reason = StopReason::ToleranceReached;
            break;
        }
    }

    // Keep memberships and means from the same completed iteration; no reassignment.
    clusters.swap(workingClusters);
    iterationCount = completedIterations;
    stoppingReason = reason;
}

const std::vector<Cluster>& KMeans::getClusters() const {
    return clusters;
}

std::size_t KMeans::getIterationCount() const {
    return iterationCount;
}

KMeans::StopReason KMeans::getStoppingReason() const {
    return stoppingReason;
}
