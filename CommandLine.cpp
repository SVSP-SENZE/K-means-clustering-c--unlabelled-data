#include "CommandLine.h"
#include "SvgPlot.h"
#include "CsvIO.h"
#include "RandomInitialiser.h"
#include "KMeansPlusPlusInitialiser.h"
#include "JsonIO.h"

#include <charconv>
#include <functional>
#include <algorithm>
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
        std::cout << "\n  FINAL CENTROIDS AND CLUSTER SIZES\n"
                  << "  ------------------------------------------------------\n";
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
        if (model.getStoppingReason() == KMeans::StopReason::IterationLimit) {
            std::cout << "  Iteration limit reached; try a higher limit to allow convergence.\n";
        }
        std::cout << "\nExported assignments.csv, centroids.csv, summary.txt and results.json to " << argv[7];
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
std::string trimInput(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r");
    if (first == std::string::npos) return "";
    return value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
}

// Each field is checked before advancing; EOF and /back cancel the wizard.
bool ask(const std::string& label, const std::string& fallback, std::string& value,
         const std::function<void(const std::string&)>& validate) {
    for (;;) {
        std::cout << "  " << label;
        if (!fallback.empty()) std::cout << " [" << fallback << "]";
        std::cout << ": " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) return false;
        line = trimInput(line);
        if (line == "/back") return false;
        if (line.empty()) line = fallback;
        try {
            validate(line);
            value = line;
            return true;
        } catch (const std::exception& error) {
            std::cout << "  ! " << error.what() << " Try again.\n";
        }
    }
}

std::string nextOutputFolder() {
    std::string name = "results";
    for (std::size_t i = 1; std::filesystem::exists(name); ++i)
        name = "results-" + std::to_string(i);
    return name;
}

int runInteractiveMenu() {
    for (;;) {
        std::cout << "\n  +------------------------------------------------------+\n"
                  << "  |  MINICLUSTER                                         |\n"
                  << "  |  Find groups in unlabeled numerical data             |\n"
                  << "  +------------------------------------------------------+\n"
                  << "\n  1  New analysis     - guided CSV / JSON setup\n"
                  << "  2  Command-line help\n"
                  << "  3  Exit\n"
                  << "  4  Quick demo       - sample.csv, two clusters\n"
                  << "  5  How it works     - data format and settings\n"
                  << "\n  Choose [1-5]: " << std::flush;
        std::string choice;
        if (!std::getline(std::cin, choice)) return 0;
        choice = trimInput(choice);
        if (choice == "3") {
            std::cout << "\n  Goodbye.\n";
            return 0;
        }
        if (choice == "2") { printUsage(); continue; }
        if (choice == "5") {
            std::cout << "\n  K-MEANS IN THREE STEPS\n"
                      << "  1. Choose k starting centroids.\n"
                      << "  2. Assign each point to its nearest centroid.\n"
                      << "  3. Recalculate the means; repeat until movement is small.\n"
                      << "\n  k is the number of groups you want (1 to the point count).\n"
                      << "  kmeans++ spreads starting centres; random samples input points.\n"
                      << "  A fixed seed makes a run repeatable. Different seeds may give\n"
                      << "  different results; K-means does not guarantee the best grouping.\n"
                      << "  Inertia is the sum of squared distances to cluster centroids.\n"
                      << "  Lower values mean a tighter fit for the same data and k.\n"
                      << "\n  CSV: numerical rows without headers, e.g. 1,2 then 8,9.\n"
                      << "  JSON: {\"points\": [[1,2], [8,9]]}\n"
                      << "  For Mall Customers, select numerical features and remove the\n"
                      << "  header, customer IDs, and text columns before importing.\n"
                      << "  Scale features beforehand if their units/ranges differ greatly.\n"
                      << "  Outputs include centroids, sizes, assignments and an SVG plot\n"
                      << "  for data with two or more features. Labels start at zero.\n";
            continue;
        }
        if (choice != "1" && choice != "4") {
            std::cout << "  ! Choose a number from 1 to 5.\n";
            continue;
        }
        try {
            std::string values[9] = {"sample.csv", "2", "kmeans++", "42", "100",
                                     "0.000001", nextOutputFolder(), "1", "2"};
            DataSet source;
            const auto readSource = [&](const std::string& input) {
                if (input.empty()) throw std::invalid_argument("Enter a file path.");
                const std::filesystem::path path(input);
                source = path.extension() == ".json" ? jsonio::readDataSet(path) : csv::readDataSet(path);
            };
            bool complete = true;
            if (choice == "4") {
                readSource(values[0]);
                std::cout << "\n  QUICK DEMO | sample.csv | k=2 | kmeans++ | seed=42\n";
            } else {
                std::cout << "\n  NEW ANALYSIS\n  Press Enter to accept [defaults]; type /back to cancel.\n\n"
                          << "  STEP 1 / 3 - Load data\n";
                complete = ask("Input file (path without quotes)", "sample.csv", values[0], readSource);
                if (complete) {
                    std::cout << "  Loaded " << source.size() << " points with " << source.dimension()
                              << " features each.\n\n  STEP 2 / 3 - Choose settings\n";
                    complete = ask("Number of clusters", std::to_string(std::min<std::size_t>(2, source.size())), values[1],
                        [&](const std::string& v) {
                            if (parseCount(v, "k") > source.size())
                                throw std::invalid_argument("k cannot exceed " + std::to_string(source.size()) + " points.");
                        }) && ask("Initialization (kmeans++ / random)", "kmeans++", values[2],
                        [](const std::string& v) {
                            if (v != "random" && v != "kmeans++") throw std::invalid_argument("Choose random or kmeans++.");
                        }) && ask("Random seed", "42", values[3], [](const std::string& v) {
                            if (parseUnsigned(v, "Seed") > std::numeric_limits<std::uint32_t>::max())
                                throw std::invalid_argument("Seed must be between 0 and 4294967295.");
                        }) && ask("Maximum iterations", "100", values[4], [](const std::string& v) { parseCount(v, "Maximum iterations"); })
                        && ask("Movement tolerance", "0.000001", values[5], [](const std::string& v) {
                            if (csv::parseFiniteNumber(v, "Tolerance") < 0) throw std::invalid_argument("Tolerance must be nonnegative.");
                        });
                }
                if (complete) {
                    std::cout << "\n  STEP 3 / 3 - Save and plot\n";
                    complete = ask("New output folder", values[6], values[6], [](const std::string& v) {
                        if (v.empty()) throw std::invalid_argument("Enter an output folder.");
                        if (std::filesystem::exists(v)) throw std::invalid_argument("That path already exists; choose a new folder.");
                    });
                    if (complete && source.dimension() >= 2) {
                        const auto checkFeature = [&](const std::string& v) {
                            if (parseCount(v, "Feature") > source.dimension()) throw std::invalid_argument("Feature number exceeds the dataset dimension.");
                        };
                        complete = ask("X-axis feature (numbered from 1)", "1", values[7], checkFeature)
                            && ask("Y-axis feature", values[7] == "2" ? "1" : "2", values[8], [&](const std::string& v) {
                                checkFeature(v);
                                if (parseCount(v, "Y feature") == parseCount(values[7], "X feature"))
                                    throw std::invalid_argument("Choose a different feature from the X axis.");
                            });
                    } else if (complete) std::cout << "  One-dimensional data: no scatter plot will be created.\n";
                }
            }
            if (!complete) {
                if (std::cin.eof()) return 0;
                std::cout << "\n  Analysis cancelled.\n";
                continue;
            }
            std::cout << "\n  ANALYSIS | " << source.size() << " points | k=" << values[1]
                      << " | " << values[2] << "\n"
                      << "  ------------------------------------------------------\n";
            char program[] = "MiniCluster";
            char* arguments[10] = {program};
            for (std::size_t i = 0; i < 9; ++i) arguments[i + 1] = values[i].data();
            const int result = runCommandLine(source.dimension() >= 2 ? 10 : 8, arguments);
            std::cout << (result == 0 ? "\n  Analysis complete. Your results are saved.\n" : "\n  Analysis failed. Review the error above.\n");
        } catch (const std::exception& error) {
            std::cout << "  ! " << error.what() << '\n';
        }
    }
}
}
