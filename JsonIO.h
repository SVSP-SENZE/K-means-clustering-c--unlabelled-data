#pragma once

#include "DataSet.h"
#include "KMeans.h"

#include <filesystem>

namespace jsonio {
DataSet readDataSet(const std::filesystem::path& filename);

void exportResults(const std::filesystem::path& filename,
                   const KMeans& model);
}