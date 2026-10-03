#pragma once

#include "KMeans.h"
#include <filesystem>
#include <cstddef>

void writeClusterPlot(const KMeans& model,
                      std::size_t xFeature,  // zero-based
                      std::size_t yFeature,  // zero-based
                      const std::filesystem::path& filename);