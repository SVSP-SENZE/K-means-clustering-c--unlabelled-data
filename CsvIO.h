#pragma once

#include "DataSet.h"
#include "KMeans.h"

#include <filesystem>
#include <string>

namespace csv {
// Shared strict number conversion for CSV fields and the CLI tolerance.
double parseFiniteNumber(const std::string& text, const std::string& context);
DataSet readDataSet(const std::filesystem::path& filename);
void exportResults(const std::filesystem::path& directory,
                   const KMeans& model, const std::string& summary);
}
