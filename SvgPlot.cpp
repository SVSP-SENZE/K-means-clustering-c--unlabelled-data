#include "SvgPlot.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

void writeClusterPlot(const KMeans& model,
                      std::size_t xFeature,
                      std::size_t yFeature,
                      const std::filesystem::path& filename) {
    const auto& clusters = model.getClusters();
    if (clusters.empty()) {
        throw std::logic_error("Fit the model before creating a plot.");
    }

    const std::size_t dimension = clusters.front().getCentroid().dimension();
    if (xFeature >= dimension || yFeature >= dimension || xFeature == yFeature) {
        throw std::invalid_argument("Choose two different features within the dataset dimension.");
    }

    struct Point2D { double x, y; };
    std::vector<std::vector<Point2D>> members(clusters.size());
    std::vector<Point2D> centroids;
    double minX = 0, maxX = 0, minY = 0, maxY = 0;
    bool first = true;

    auto include = [&](double x, double y) {
        if (first) {
            minX = maxX = x;
            minY = maxY = y;
            first = false;
        } else {
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
        }
    };

    for (std::size_t i = 0; i < clusters.size(); ++i) {
        for (const DataPoint& point : clusters[i].getMembers()) {
            const auto& f = point.getFeatures();
            members[i].push_back({f[xFeature], f[yFeature]});
            include(f[xFeature], f[yFeature]);
        }
        const auto& f = clusters[i].getCentroid().getFeatures();
        centroids.push_back({f[xFeature], f[yFeature]});
        include(f[xFeature], f[yFeature]);
    }

    // Pad the plot bounds; also handles a feature whose values are all equal.
    double spanX = maxX - minX;
    double spanY = maxY - minY;
    if (spanX == 0) spanX = 1;
    if (spanY == 0) spanY = 1;
    minX -= spanX * 0.1; maxX += spanX * 0.1;
    minY -= spanY * 0.1; maxY += spanY * 0.1;

    constexpr double left = 80, top = 50, width = 720, height = 480;
    auto px = [&](double x) { return left + (x - minX) / (maxX - minX) * width; };
    auto py = [&](double y) { return top + height - (y - minY) / (maxY - minY) * height; };

    const char* colors[] = {"#1976d2", "#e53935", "#43a047", "#8e24aa",
                            "#fb8c00", "#00897b", "#6d4c41", "#3949ab"};

    std::ofstream out(filename);
    if (!out) throw std::runtime_error("Could not create plot: " + filename.string());

    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"900\" height=\"620\" "
           "viewBox=\"0 0 900 620\">\n"
           "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n"
           "<text x=\"450\" y=\"28\" text-anchor=\"middle\" font-size=\"20\">"
           "K-means clusters</text>\n"
           "<line x1=\"80\" y1=\"530\" x2=\"800\" y2=\"530\" stroke=\"black\"/>\n"
           "<line x1=\"80\" y1=\"50\" x2=\"80\" y2=\"530\" stroke=\"black\"/>\n";

    for (std::size_t i = 0; i < members.size(); ++i) {
        const char* color = colors[i % (sizeof(colors) / sizeof(colors[0]))];
        for (const Point2D& p : members[i]) {
            out << "<circle cx=\"" << px(p.x) << "\" cy=\"" << py(p.y)
                << "\" r=\"6\" fill=\"" << color << "\"/>\n";
        }
        const Point2D c = centroids[i];
        out << "<path d=\"M " << px(c.x) - 9 << ' ' << py(c.y)
            << " L " << px(c.x) + 9 << ' ' << py(c.y)
            << " M " << px(c.x) << ' ' << py(c.y) - 9
            << " L " << px(c.x) << ' ' << py(c.y) + 9
            << "\" stroke=\"black\" stroke-width=\"3\"/>\n";
        out << "<text x=\"" << px(c.x) + 10 << "\" y=\"" << py(c.y) - 8
            << "\" font-size=\"14\">C" << i << "</text>\n";
    }

    out << "<text x=\"440\" y=\"590\" text-anchor=\"middle\">Feature "
        << (xFeature + 1) << "</text>\n"
        << "<text x=\"20\" y=\"290\" text-anchor=\"middle\" "
           "transform=\"rotate(-90 20 290)\">Feature "
        << (yFeature + 1) << "</text>\n"
        << "</svg>\n";

    if (!out) throw std::runtime_error("Failed while writing plot: " + filename.string());
}