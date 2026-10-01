# MiniCluster design

## Milestone workflow

Read existing code and repository instructions before each numbered milestone.
Preserve working behaviour, implement only the requested scope, and use
beginner-readable C++17 and the standard library. Explain unfamiliar language
features and report design deviations. Do not push to GitHub without a request.

## Seven-class design

| Class | Responsibility | Current status |
| --- | --- | --- |
| DataPoint | Own one numerical feature vector and calculate squared distance | Implemented in milestone 1 |
| DataSet | Own points of a consistent dimension | Implemented in milestone 1 |
| Cluster | Own a centroid and assigned members | Not implemented |
| KMeans | Coordinate clustering and prediction | Not implemented |
| IInitialiser | Define the centroid-selection interface | Not implemented |
| RandomInitialiser | Select random initial centroids | Not implemented |
| KMeansPlusPlusInitialiser | Select initial centroids using K-Means++ | Not implemented |

The interface filename is **IInitializer.h** and its future class name is
**IInitialiser**. Do not create a duplicate interface file.

## Milestone 1 decisions

- DataPoint privately owns std::vector<double>, copied from the constructor
  argument using a member initializer list. Empty and non-finite features throw
  std::invalid_argument. No default constructor is provided.
- dimension() returns the feature count. getFeatures() returns a const reference.
  squaredDistanceTo() rejects unequal dimensions before summing squared differences.
- DataSet privately owns std::vector<DataPoint> and starts empty. dimension()
  returns zero when empty and otherwise reads the first point's dimension.
  addPoint() checks dimensions before copying the point into the vector.
  getPoints() returns a const reference; size() returns the point count.
- main.cpp demonstrates four 2D points, squared distance, and rejection of a 3D
  point. Final centroids and cluster sizes belong to future milestones.

No milestone 1 design deviations were identified. The example includes an extra
line showing dataset size after rejection.
