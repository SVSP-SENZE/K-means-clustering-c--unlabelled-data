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
| Cluster | Own a centroid and assigned members | Implemented in milestone 2 |
| KMeans | Coordinate clustering; prediction is future scope | fit and results implemented in milestone 4 |
| IInitialiser | Define the centroid-selection interface | Implemented in milestone 3 |
| RandomInitialiser | Select random initial centroids | Implemented in milestone 3 |
| KMeansPlusPlusInitialiser | Select initial centroids using K-Means++ | Not implemented |

The interface filename is **IInitializer.h** and its class name is
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

## Milestone 2 decisions

- Cluster owns a private DataPoint centroid and std::vector<DataPoint> members.
  Its explicit constructor copies the initial centroid through a member initializer
  list. Membership starts empty; there is no default constructor.
- addMember() validates against the centroid dimension before adding a copy.
  clearMembers() preserves the centroid. Const reference getters expose read-only
  access, and size() reports the member count.
- updateCentroid() sums each coordinate and divides by the number of members.
  It constructs a replacement DataPoint before assigning the centroid, so failed
  validation leaves the previous centroid intact. Empty updates do nothing.
- The straightforward double-precision sum can overflow for extreme finite
  inputs; a non-finite result is rejected by DataPoint's existing validation.
- main.cpp retains milestone 1 and adds six self-checking Cluster scenarios with
  an absolute tolerance of 1e-9. A failed check throws, is reported, and returns 1.

No milestone 2 design deviations. Initializers and KMeans remain unimplemented.

## Milestone 3 decisions

- IInitializer.h defines IInitialiser with a public inline defaulted virtual
  destructor and a const pure virtual initialise(dataset, k, rng) operation.
- RandomInitialiser publicly inherits from IInitialiser and overrides that
  operation. Empty input and k outside [1, dataset.size()] throw
  std::invalid_argument before shuffling.
- Indices are filled with std::iota, shuffled using the supplied std::mt19937,
  and the first k indexed points are copied into the result. Distinct indices
  may refer to identical coordinates. The generator is never reseeded internally.
- The const operation leaves the initializer unchanged but advances the caller's
  generator through a non-const reference. Same-seed tests use fresh generators
  and compare ordered results within this implementation; no cross-library
  shuffle ordering or different-seed difference is assumed.
- Planned lifetime: main() owns the initializer. A future KMeans::fit() receives
  it by const reference for the duration of the call. KMeans does not retain or
  own the initializer.

No milestone 3 design deviations. KMeans and KMeansPlusPlusInitialiser remain
unimplemented. The milestone-1 and milestone-2 demonstrations are preserved.

## Milestone 4 decisions

- KMeans(k, maximumIterations, tolerance, seed) requires positive counts and a
  finite, nonnegative tolerance. Counts use std::size_t. fit() rejects empty data
  and k greater than dataset size. DataPoint/DataSet already enforce finite
  features and consistent dimensions. Initializer output must contain exactly k
  finite centroids of the dataset dimension; centres need not be input points
  or have distinct coordinates.
- fit(const DataSet&, const IInitialiser&) creates a fresh local std::mt19937
  from the configured seed each call. main() owns the strategy; KMeans neither
  retains nor owns it. Repeated calls restart the generator, with reproducibility
  subject to the initializer's behaviour and the same library implementation.
- Each iteration clears members, assigns all points using squared distance,
  then updates means. Strictly smaller distances replace the nearest index, so
  ties choose the lowest index. Cluster preserves centroids of empty clusters.
- Movement is sqrt(squaredDistanceTo(previousCentroid)), compared directly with
  tolerance. The maximum movement must be <= tolerance. This avoids squaring
  very large finite tolerances. Non-finite squared distances or movement cause
  std::overflow_error; the existing centroid sum overflow limitation remains.
- Results are built locally and published only after a successful fit. Failed
  fits leave previous results and metadata unchanged. getClusters() returns a
  const reference; getIterationCount() and getStoppingReason() are const getters.
  Initial status is NotFitted with no clusters and zero completed iterations.
- StopReason is an enum class: NotFitted, ToleranceReached, IterationLimit.
  Iterations count completed assignment/update pairs. If both stop conditions
  apply on the same iteration, ToleranceReached takes precedence.
- Reported memberships and centroids come from the same completed iteration.
  There is no final reassignment: each nonempty centroid is its reported members'
  mean. A tolerance or iteration-limit stop does not guarantee an exact assignment
  fixed point: assigning again against the new centroids can move some points.
- FixedInitialiser exists only in main.cpp as a test helper, including malformed
  output scenarios. It does not extend the seven production classes. The demo
  prints every final centroid, size, iteration count, and stopping reason.

No requested milestone-4 functionality was omitted. Implementation choices beyond
the minimum are preserving results after failed fits and rejecting non-finite
distance arithmetic. Prediction and KMeansPlusPlusInitialiser remain future work.
