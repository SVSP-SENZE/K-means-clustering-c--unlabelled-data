#include "CommandLine.h"
#include "SvgPlot.h"
#include "CsvIO.h"
#include "RandomInitialiser.h"
#include "KMeansPlusPlusInitialiser.h"
#include "JsonIO.h"

#include <charconv>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
int runInteractiveMenu();

void printUsage() {
    std::cout << "Usage: MiniCluster.exe INPUT.csv K random|kmeans++ SEED MAX_ITERATIONS TOLERANCE OUTPUT_DIR [X_FEATURE Y_FEATURE]\n"
              << "Example: MiniCluster.exe sample.csv 2 kmeans++ 42 100 0.000001 results\n"
              << "INPUT: no header, finite comma-separated numbers; blank lines are ignored.\n"
              << "OUTPUT_DIR must be new. Cluster indices start at 0.\n"
              << "Run MiniClusterTests.exe separately for the C++ test suite.\n";
}

std::uint64_t parseUnsigned(const std::string& text, const std::string& name) {
    std::uint64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        throw std::invalid_argument(name + " must be an unsigned decimal integer in range.");
    }
    return value;
}

std::size_t parseCount(const std::string& text, const std::string& name) {
    std::uint64_t value = parseUnsigned(text, name);
    if (value == 0 || value > std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument(name + " must be positive and fit in std::size_t.");
    }
    return static_cast<std::size_t>(value);
}
}

int runCommandLine(int argc, char* argv[]) {
    if (argc == 1) {
        return runInteractiveMenu();
    }
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            printUsage();
            return 0;
        }
        if (argc != 8 && argc != 10) {
            printUsage();
            throw std::invalid_argument("Expected seven arguments and optional X/Y plot features; use --help for the format.");
        }
        const std::string inputFile = argv[1];
        const std::size_t k = parseCount(argv[2], "k");
        const std::string method = argv[3];
        if (method != "random" && method != "kmeans++") {
            throw std::invalid_argument("Initialization method must be random or kmeans++.");
        }
        const std::uint64_t seedValue = parseUnsigned(argv[4], "Seed");
        if (seedValue > std::numeric_limits<std::uint32_t>::max()) {
            throw std::invalid_argument("Seed must be between 0 and 4294967295.");
        }
        const auto seed = static_cast<std::mt19937::result_type>(seedValue);
        const std::size_t maximumIterations = parseCount(argv[5], "Maximum iterations");
        const double tolerance = csv::parseFiniteNumber(argv[6], "Tolerance");
        KMeans model(k, maximumIterations, tolerance, seed);
        const std::filesystem::path inputPath = inputFile;
        DataSet dataset;

        if (inputPath.extension() == ".json") {
            dataset = jsonio::readDataSet(inputPath);
        } else {
            dataset = csv::readDataSet(inputPath);
        }
        std::size_t xFeature = 0;
        std::size_t yFeature = 1;
        if (argc == 10) {
            xFeature = parseCount(argv[8], "X feature") - 1;
            yFeature = parseCount(argv[9], "Y feature") - 1;
            if (dataset.dimension() < 2 || xFeature >= dataset.dimension() ||
                yFeature >= dataset.dimension() || xFeature == yFeature) {
                throw std::invalid_argument("Plot features must be two different features between 1 and " +
                                            std::to_string(dataset.dimension()) + ".");
            }
        }
        RandomInitialiser random;
        KMeansPlusPlusInitialiser plusPlus;
        if (method == "random") {
            model.fit(dataset, random);
        } else {
            model.fit(dataset, plusPlus);
        }

        std::ostringstream summary;
        summary.imbue(std::locale::classic());
        summary << std::setprecision(std::numeric_limits<double>::max_digits10)
                << "input_file: " << inputFile << '\n'
                << "points: " << dataset.size() << '\n'
                << "dimension: " << dataset.dimension() << '\n'
                << "k: " << k << '\n'
                << "initialization: " << method << '\n'
                << "seed: " << seedValue << '\n'
                << "maximum_iterations: " << maximumIterations << '\n'
                << "tolerance: " << tolerance << '\n'
                << "inertia: " << model.getInertia() << '\n'
                << "iterations: " << model.getIterationCount() << '\n'
                << "stopping_reason: "
                << (model.getStoppingReason() == KMeans::StopReason::ToleranceReached
                    ? "tolerance_reached" : "iteration_limit") << '\n'
                << "cluster_numbering: zero-based (0 to k-1)\n"
                << "assignment_policy: reported members from the final completed iteration\n";
        
        csv::exportResults(argv[7], model, summary.str());

        jsonio::exportResults(
            std::filesystem::path(argv[7]) / "results.json",
            model);

        if (dataset.dimension() >= 2) {
            writeClusterPlot(
                model, xFeature, yFeature,
                std::filesystem::path(argv[7]) / "clusters.svg");
        }
        std::cout << summary.str();
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
        std::cout << "Exported CSV files and results.json to " << argv[7];
        if (dataset.dimension() >= 2) {
            std::cout << " (also clusters.svg using features " << (xFeature + 1)
                      << " and " << (yFeature + 1) << ")";
        }
        std::cout << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}

namespace {
int runInteractiveMenu() {
    for (;;) {
        std::cout << "\nMiniCluster - K-means\n"
                  << "1. Run a clustering analysis\n"
                  << "2. Show command-line help\n"
                  << "3. Exit\n"
                  << "Choose an option: ";
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            std::cout << "\nGoodbye.\n";
            return 0;
        }
        if (choice == "3") {
            std::cout << "Goodbye.\n";
            return 0;
        }
        if (choice == "2") {
            printUsage();
            continue;
        }
        if (choice != "1") {
            std::cout << "Please enter 1, 2, or 3.\n";
            continue;
        }

        const char* prompts[] = {
            "Input file (CSV or JSON): ", "Number of clusters (k): ",
            "Initialization method (random or kmeans++): ", "Random seed (0-4294967295): ",
            "Maximum iterations: ", "Tolerance (for example 0.000001): ",
            "New output folder: "
        };
        std::string values[9];
        bool complete = true;
        for (std::size_t i = 0; i < 7; ++i) {
            std::cout << prompts[i];
            if (!std::getline(std::cin, values[i])) {
                complete = false;
                break;
            }
        }
        if (!complete) {
            std::cout << "\nInput cancelled. Returning to the menu.\n";
            if (std::cin.eof()) return 0;
            continue;
        }

        DataSet source;
        try {
            const std::filesystem::path inputPath(values[0]);
            source = inputPath.extension() == ".json"
                ? jsonio::readDataSet(inputPath) : csv::readDataSet(inputPath);
        } catch (const std::exception& error) {
            std::cerr << "ERROR: " << error.what() << '\n';
            continue;
        }

        if (source.dimension() >= 2) {
            std::cout << "This file has " << source.dimension() << " features (numbered 1 through "
                      << source.dimension() << ").\n";
            std::cout << "X-axis feature number: "<<std::endl;
            std::string xFeature;
            std::cout << "Y-axis feature number: "<<std::endl;
            std::string yFeature;
            if (!std::getline(std::cin, xFeature) || !std::getline(std::cin, yFeature)) {
                std::cout << "\nInput cancelled.\n";
                return 0;
            }
            values[7] = xFeature;
            values[8] = yFeature;
        } else {
            std::cout << "This file has fewer than 2 features, so no graph will be created.\n";
        }

        char program[] = "MiniCluster";
        char* arguments[10] = {program};
        for (std::size_t i = 0; i < 7; ++i) arguments[i + 1] = values[i].data();
        const bool hasPlotFeatures = source.dimension() >= 2;
        if (hasPlotFeatures) {
            arguments[8] = values[7].data();
            arguments[9] = values[8].data();
        }
        const int result = runCommandLine(hasPlotFeatures ? 10 : 8, arguments);
        if (result != 0) std::cout << "Please review the error above and try again.\n";
    }
}
}
