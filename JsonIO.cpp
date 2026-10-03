#include <cmath>
#include "JsonIO.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

using nlohmann::json;

DataSet jsonio::readDataSet(const std::filesystem::path& filename) {
    std::ifstream input(filename);
    if (!input) {
        throw std::runtime_error("Cannot open JSON input file: " + filename.string());
    }

    json document;
    try {
        input >> document;
    } catch (const json::parse_error& error) {
        throw std::runtime_error("Invalid JSON: " + std::string(error.what()));
    }

    if (!document.is_object() ||
        !document.contains("points") ||
        !document["points"].is_array() ||
        document["points"].empty()) {
        throw std::invalid_argument(
            "JSON input must contain a nonempty 'points' array.");
    }

    DataSet dataset;
    std::size_t expectedDimension = 0;

    for (std::size_t row = 0; row < document["points"].size(); ++row) {
        const json& values = document["points"][row];
        if (!values.is_array() || values.empty()) {
            throw std::invalid_argument(
                "Each item in 'points' must be a nonempty array of numbers.");
        }

        std::vector<double> features;
        for (const json& value : values) {
            if (!value.is_number()) {
                throw std::invalid_argument(
                    "Every feature value in 'points' must be numeric.");
            }
            const double number = value.get<double>();
            if (!std::isfinite(number)) {
                throw std::invalid_argument(
                    "Feature values must be finite numbers.");
            }
            features.push_back(number);
        }

        if (row == 0) {
            expectedDimension = features.size();
        } else if (features.size() != expectedDimension) {
            throw std::invalid_argument(
                "All rows in 'points' must have the same number of features.");
        }

        dataset.addPoint(DataPoint(features));
    }

    return dataset;
}

void jsonio::exportResults(const std::filesystem::path& filename,
                           const KMeans& model) {
    json output;
    output["inertia"] = model.getInertia();
    output["iterations"] = model.getIterationCount();
    output["clusters"] = json::array();

    const auto& clusters = model.getClusters();
    if (clusters.empty()) {
        throw std::logic_error("Cannot export JSON before a successful fit.");
    }
    output["dimension"] = clusters.front().getCentroid().dimension();
    output["cluster_count"] = clusters.size();

    for (std::size_t i = 0; i < clusters.size(); ++i) {
        const Cluster& cluster = clusters[i];
        json clusterJson;
        clusterJson["cluster_index"] = i;
        clusterJson["size"] = cluster.size();
        clusterJson["centroid"] = cluster.getCentroid().getFeatures();
        clusterJson["members"] = json::array();

        for (const DataPoint& point : cluster.getMembers()) {
            clusterJson["members"].push_back(point.getFeatures());
        }

        output["clusters"].push_back(clusterJson);
    }

    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("Cannot create JSON output file: " + filename.string());
    }

    file << output.dump(2) << '\n';
    if (!file) {
        throw std::runtime_error("Failed writing JSON output file: " + filename.string());
    }
}
