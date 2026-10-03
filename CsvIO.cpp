#include "CsvIO.h"

#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <stdexcept>

namespace {
std::string trim(const std::string& text) {
    std::size_t first = text.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return "";
    }
    std::size_t last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

std::ofstream openOutput(const std::filesystem::path& filename) {
    std::ofstream output(filename);
    if (!output) {
        throw std::runtime_error("Cannot create output file: " + filename.string());
    }
    output.imbue(std::locale::classic());
    output << std::setprecision(std::numeric_limits<double>::max_digits10);
    return output;
}

void finishOutput(std::ofstream& output, const std::filesystem::path& filename) {
    output.close();
    if (!output) {
        throw std::runtime_error("Failed writing output file: " + filename.string());
    }
}

void writeFeatures(std::ostream& output, const DataPoint& point) {
    const std::vector<double>& features = point.getFeatures();
    for (std::size_t i = 0; i < features.size(); ++i) {
        if (i != 0) {
            output << ',';
        }
        output << features[i];
    }
}
}

double csv::parseFiniteNumber(const std::string& text, const std::string& context) {
    const std::string value = trim(text);
    if (value.empty()) {
        throw std::invalid_argument(context + ": empty field; expected a finite number.");
    }
    const char* begin = value.data();
    const char* end = begin + value.size();
    // from_chars accepts a leading minus, but requires us to handle plus ourselves.
    if (*begin == '+') {
        ++begin;
        if (begin == end || *begin == '-' || *begin == '+') {
            throw std::invalid_argument(context + ": invalid number '" + value + "'.");
        }
    }
    double number = 0.0;
    const auto result = std::from_chars(begin, end, number, std::chars_format::general);
    if (result.ec == std::errc::result_out_of_range) {
        throw std::invalid_argument(context + ": number is outside the double range: '" + value + "'.");
    }
    if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(number)) {
        throw std::invalid_argument(context + ": expected a finite decimal number, got '" + value + "'.");
    }
    return number;
}

DataSet csv::readDataSet(const std::filesystem::path& filename) {
    if (!std::filesystem::is_regular_file(filename)) {
        throw std::runtime_error("Input is missing or is not a regular file: " + filename.string());
    }
    std::ifstream input(filename, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Cannot open input file: " + filename.string());
    }

    DataSet dataset;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back(); // Support CRLF as well as LF line endings.
        }
        if (trim(line).empty()) {
            continue;
        }
        const std::string context = filename.string() + ": line " + std::to_string(lineNumber);
        std::vector<double> features;
        std::size_t start = 0;
        while (true) {
            std::size_t comma = line.find(',', start);
            std::string field = line.substr(start, comma == std::string::npos
                ? std::string::npos : comma - start);
            features.push_back(parseFiniteNumber(field,
                context + ", field " + std::to_string(features.size() + 1)));
            if (comma == std::string::npos) {
                break;
            }
            start = comma + 1;
        }
        if (dataset.size() != 0 && features.size() != dataset.dimension()) {
            throw std::invalid_argument(context + ": expected " + std::to_string(dataset.dimension())
                + " fields, got " + std::to_string(features.size()) + ".");
        }
        dataset.addPoint(DataPoint(features));
    }
    if (!input.eof() || input.bad()) {
        throw std::runtime_error("Failed reading input file: " + filename.string());
    }
    if (dataset.size() == 0) {
        throw std::invalid_argument("Input contains no data points: " + filename.string());
    }
    return dataset;
}

void csv::exportResults(const std::filesystem::path& directory,
                        const KMeans& model, const std::string& summary) {
    if (model.getStoppingReason() == KMeans::StopReason::NotFitted) {
        throw std::logic_error("Cannot export results before a successful fit.");
    }
    if (std::filesystem::exists(directory)) {
        throw std::runtime_error("Output directory already exists; choose a new directory: "
                                 + directory.string());
    }
    if (!std::filesystem::create_directories(directory)) {
        throw std::runtime_error("Cannot create output directory: " + directory.string());
    }
    const std::vector<Cluster>& clusters = model.getClusters();
    std::size_t dimension = clusters.front().getCentroid().dimension();
    const std::filesystem::path pointsPath = directory / "assignments.csv";
    std::ofstream points = openOutput(pointsPath);
    for (std::size_t i = 0; i < dimension; ++i) {
        points << "feature_" << i + 1 << ',';
    }
    points << "cluster_index\n";
    for (std::size_t i = 0; i < clusters.size(); ++i) {
        // Export the fitted members, not fresh predict() assignments.
        for (const DataPoint& member : clusters[i].getMembers()) {
            writeFeatures(points, member);
            points << ',' << i << '\n';
        }
    }
    finishOutput(points, pointsPath);

    const std::filesystem::path centroidsPath = directory / "centroids.csv";
    std::ofstream centroids = openOutput(centroidsPath);
    centroids << "cluster_index,";
    for (std::size_t i = 0; i < dimension; ++i) {
        centroids << "feature_" << i + 1 << ',';
    }
    centroids << "size\n";
    for (std::size_t i = 0; i < clusters.size(); ++i) {
        centroids << i << ',';
        writeFeatures(centroids, clusters[i].getCentroid());
        centroids << ',' << clusters[i].size() << '\n';
    }
    finishOutput(centroids, centroidsPath);

    const std::filesystem::path summaryPath = directory / "summary.txt";
    std::ofstream summaryOutput = openOutput(summaryPath);
    summaryOutput << summary;
    finishOutput(summaryOutput, summaryPath);
}
