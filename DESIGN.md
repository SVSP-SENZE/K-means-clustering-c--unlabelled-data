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
| KMeans | Coordinate clustering, inspect results, and predict cluster indices | fit/results in milestone 4; inertia/prediction in milestone 6 |
| IInitialiser | Define the centroid-selection interface | Implemented in milestone 3 |
| RandomInitialiser | Select random initial centroids | Implemented in milestone 3 |
| KMeansPlusPlusInitialiser | Select initial centroids using K-Means++ | Implemented in milestone 5 |

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

## Milestone 5 decisions

- KMeansPlusPlusInitialiser publicly implements IInitialiser from IInitializer.h.
  It uses the same empty-dataset and invalid-k validation as RandomInitialiser.
- The first index is sampled uniformly with std::uniform_int_distribution.
  Selected indices are tracked separately from coordinates, allowing duplicate
  observations while preventing selection of the same index twice.
- For each subsequent centre, distances to all selected centres are recomputed
  for each unselected observation. The minimum squared distance is its weight.
  This direct implementation prioritizes readability over cached distances.
- Positive weights and their dataset indices are stored in parallel vectors.
  std::discrete_distribution samples a position proportionally to its weight;
  that position is mapped back to the dataset index. Zero weights are excluded
  while any positive weight remains.
- If every remaining weight is zero, sample uniformly from all unselected
  indices using the supplied generator. No internal seeding occurs in either
  branch. Returned points are copies.
- Positive weights are divided by their maximum before sampling. A common scale
  preserves mathematical proportions and prevents overflow of the weight sum.
  Individual non-finite squared distances throw std::overflow_error, consistent
  with KMeans. Sampling remains subject to floating-point precision.
- main.cpp demonstrates the same KMeans object calling fit(dataset, random)
  and then fit(dataset, plusPlus). KMeans.h/.cpp and the core loop are unchanged.

No specification deviations. Weight scaling and explicit overflow rejection are
numerical safeguards. Prediction remains future scope.

## Milestone 6 decisions

- Two const operations extend KMeans: getInertia() returns double and
  predict(const DataPoint&) returns std::size_t. Existing const result getters
  remain the inspection API; no mutable access or redundant fitted flag is added.
- Inertia is calculated on demand by summing squared distances from each reported
  member to its reported centroid. Empty clusters contribute zero. It is not
  recomputed through predict(), and no cached value can become stale after refit.
  Cost is proportional to the number of stored points times their dimension.
- predict() compares squared distances to learned centroids and chooses the
  lowest index on ties, consistently with fit(). It neither retrains nor adds
  the input point to membership. The result is a zero-based cluster index, not
  a known semantic class label. Indices can change between fits/initializers.
- Before any successful fit, both operations throw std::logic_error with a clear
  message. Prediction dimension mismatch throws std::invalid_argument against
  the most recently fitted dimension. Non-finite prediction distance or inertia
  sum throws std::overflow_error without changing model state.
- Failure policy is unchanged and now explicitly tested: an unsuccessful first
  fit leaves no clusters, zero iterations, and NotFitted status. A failed refit
  preserves all previous successful results, metadata, inertia, and prediction
  availability. This also applies to failure during assignment, not just input
  validation. fit() continues to build locally and publish only on success.
- Successful refits replace clusters and memberships, including when the new
  dataset has a different size or dimension. They do not append old observations.
- A successful fit means a completed tolerance/iteration-limit run, not an exact
  assignment fixed point. After an early stop, predict(member) may differ from
  its reported membership; inertia intentionally measures the reported grouping.
- Inertia overflow is an inspection error: a completed fit can remain usable for
  prediction even if its total squared error exceeds the range of double.

No specification deviations. The assignment/update loop and initializer
ownership are unchanged. Previous milestone entries describe their historical scope.

## Milestone 7 decisions

- CsvIO.h/.cpp provide free functions for strict numeric conversion, dataset
  reading, and exporting results. CommandLine.h/.cpp provide CLI orchestration.
  These stateless responsibilities do not need new classes; the seven domain
  classes and clustering calculations remain unchanged.
- The CLI uses seven positional arguments: input file, k, random|kmeans++, seed,
  maximum iterations, tolerance, and output directory. All are explicit. Seed
  accepts 0 through 4294967295; counts are positive decimal integers fitting
  std::size_t. --help explains the syntax. No arguments or --self-test retains
  all previous demos/checks. Runtime/input errors return 1 with an ERROR message.
- Input has no header, quotes, comments, or BOM. Each nonblank row contains one
  point, with comma-separated finite decimal numbers in a consistent positive
  dimension. Signed decimal/scientific notation and surrounding spaces/tabs are
  accepted. Entire fields must parse; overflow/underflow and non-finite values
  are rejected. Blank or spaces/tabs-only lines are ignored; LF and CRLF work.
  Error locations use physical line numbers and one-based field numbers.
- Missing/unreadable files, directory inputs, empty/blank-only datasets, empty
  fields, malformed fields, and inconsistent dimensions fail before fitting.
  README.md is the complete format/CLI reference and sample.csv is the fixture.
- assignments.csv has feature_1,...,feature_D,cluster_index; it iterates the
  model's stored members in cluster order and preserves duplicates. It does not
  promise original global row order and does not call predict(). centroids.csv
  has cluster_index,feature_1,...,feature_D,size, including empty clusters.
- summary.txt contains the input filename, configuration, point count/dimension,
  inertia, completed iterations, stopping reason, numbering, and assignment
  policy. Input is headerless; output CSVs have documented headers and metadata.
- Cluster indices are zero-based everywhere, matching predict(). They are not
  semantic labels. Number formatting uses the classic locale and max_digits10
  precision. Exported numbers can round-trip to double.
- Outputs require a new directory; existing paths are never overwritten. File
  open/write/close errors are checked. An I/O failure may leave partial files in
  the newly created directory; multi-file atomic export is not implemented.
- Integration checks in test_csv_cli.py use only Python's standard library to
  exercise the executable with temporary fixtures. Python is not an application
  dependency. The sample-results directory records an actual successful run.

No specification deviations. Assignments are intentionally grouped by cluster;
preserving input row identifiers was not requested and would need additional
bookkeeping beyond the existing member representation.
