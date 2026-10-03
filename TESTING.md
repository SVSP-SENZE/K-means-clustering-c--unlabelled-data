# MiniCluster testing

Current suite: build/run MiniClusterTests.exe using the milestone-8 commands at
the end of this file. main.cpp is now CLI-only; older --self-test instructions
below are historical records. README.md contains the current quick-start.

For each implementation milestone, compile with warnings enabled and run
meaningful checks. Keep executed results separate from proposed checks.

## Executed during milestone 1

Compiler: g++.exe (Rev11, Built by MSYS2 project) 15.2.0.
Commands run in the project directory using PowerShell:

```powershell
g++ --version
g++ -std=c++17 -Wall -Wextra -Wpedantic DataPoint.cpp DataSet.cpp main.cpp -o MiniCluster.exe
.\MiniCluster.exe
```

The initial build failed because DataPoint.h contained a duplicate class
declaration. The duplicate was removed. The final build exited with code 0 and
no compiler warnings. The example exited with code 0 and printed:

```text
Dataset size: 4
Dataset dimension: 2
Squared distance between (1,1) and (1,3): 4
Rejected 3D point: Point dimension does not match dataset dimension.
Dataset size after rejected addition: 4
```

These executed checks cover the example's point insertion, dimension reporting,
squared distance, and unchanged dataset size after a rejected addition.
git diff --check also found no whitespace errors; Git printed line-ending notices.

## Proposed checks (not executed)

- Empty DataSet reports size 0 and dimension 0.
- DataPoint rejects an empty vector, NaN, and positive or negative infinity.
- Distance calculations reject mismatched dimensions.
- Valid 1D and higher-dimensional points produce the expected squared distances.
- A point owns a copy independent of the vector supplied to its constructor.

The documentation-only follow-up reviewed current code and recorded the earlier
results above; it did not recompile or rerun the executable.

## Executed during milestone 2

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic DataPoint.cpp DataSet.cpp Cluster.cpp main.cpp -o MiniCluster.exe
.\MiniCluster.exe
```

Compilation exited with code 0 and no warnings. The executable exited with code 0:

```text
Dataset size: 4
Dataset dimension: 2
Squared distance between (1,1) and (1,3): 4
Rejected 3D point: Point dimension does not match dataset dimension.
Dataset size after rejected addition: 4
PASS: Initial centroid (0,0), size 0
PASS: 2D mean centroid (1,2), size 2
PASS: 3D member rejected; centroid and members unchanged
PASS: Clear members: size 0, centroid remains (1,2)
PASS: Empty update preserves centroid (1,2)
PASS: 3D mean centroid (2,3,4), size 2
All milestone-2 checks passed.
```

The six Cluster checks execute in main.cpp without an external framework.
Coordinate comparisons use an absolute tolerance of 1e-9. Rejected insertion
checks the exception, size, centroid, and both existing members. Failed checks
throw std::runtime_error; the surrounding catch prints FAIL and returns 1.
No deliberate failure was injected to execute that failure-reporting path.
The earlier proposed DataPoint/DataSet edge-case checks remain unexecuted.

## Executed during milestone 3

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp main.cpp -o MiniCluster.exe
.\MiniCluster.exe
```

Compilation exited with code 0 and no warnings. Execution exited with code 0.
Full actual output:

```text
Dataset size: 4
Dataset dimension: 2
Squared distance between (1,1) and (1,3): 4
Rejected 3D point: Point dimension does not match dataset dimension.
Dataset size after rejected addition: 4
PASS: Initial centroid (0,0), size 0
PASS: 2D mean centroid (1,2), size 2
PASS: 3D member rejected; centroid and members unchanged
PASS: Clear members: size 0, centroid remains (1,2)
PASS: Empty update preserves centroid (1,2)
PASS: 3D mean centroid (2,3,4), size 2
All milestone-2 checks passed.
PASS: Base-reference call returns k input points
PASS: Unique input coordinates are selected without repetition
PASS: Fresh generators with the same seed give the same ordered centres
PASS: k=1 returns one input point
PASS: k=dataset.size() returns every input point once
PASS: Empty dataset is rejected
PASS: k=0 is rejected
PASS: k larger than dataset size is rejected
PASS: Identical coordinates are accepted for valid k
All milestone-3 checks passed.
```

Nine new checks run through a const IInitialiser reference. They reuse the
absolute coordinate tolerance of 1e-9 and the existing failure handling (FAIL
message and exit status 1). Same-seed repeatability compares freshly constructed
generators seeded with 12345, including centre order. No test requires different
seeds to give different results. The full-size selection check verifies all
unique observations occur once. The duplicate dataset contains three identical
observations and requests all three.

The invalid-input exception paths were executed. A deliberately failing check
was not injected. Previously proposed DataPoint/DataSet checks remain unexecuted.

## Executed during milestone 4

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp KMeans.cpp main.cpp -o MiniCluster.exe
.\MiniCluster.exe
```

Compilation exited with code 0 and no warnings. Execution exited with code 0.
The milestone-1 through milestone-3 output matched the preceding record. The
additional actual output was:

```text
PASS: New KMeans reports not fitted
PASS: Fixed centres reach (1,2) and (8,9), sizes 2 and 2, in two iterations
Fixed-initializer demo:
Iterations: 2; stop: tolerance reached
Cluster 0: centroid (1,2), size 2
Cluster 1: centroid (8,9), size 2
PASS: Repeated random fits restart the seeded generator and replace results
Random-initializer demo (seed 42):
Iterations: 2; stop: tolerance reached
Cluster 0: centroid (1,2), size 2
Cluster 1: centroid (8,9), size 2
PASS: Movement equal to tolerance stops, including at the iteration limit
PASS: Euclidean movement 0.75 exceeds tolerance 0.6
PASS: Ties choose index 0; empty cluster preserves its centroid
PASS: Iteration-limit stop preserves the completed assignment/update pair
PASS: Tolerance stop also preserves members without final reassignment
PASS: 3D fit with zero tolerance reaches the mean
PASS: Invalid k, iteration count, and tolerances are rejected
PASS: Empty dataset and k greater than dataset size are rejected
PASS: Initializer output count and dimensions are validated
PASS: Rejected fits preserve the previous completed results
All milestone-4 checks passed.
```

Thirteen new checks cover initial status, expected fixed-centre results, random
refit reproducibility including member order, stopping boundaries and units,
lowest-index ties, empty clusters, 1D/2D/3D operation, invalid configuration,
invalid dataset size, malformed initializer output, and preservation after failed
fits. Comparisons retain the 1e-9 coordinate tolerance and nonzero failure handling.

The early-stop dataset is [0, 2, 3, 10], starting centres [0, 2]. After one
iteration the members are [0] and [2, 3, 10], with means 0 and 5. Point 2 would
switch clusters on reassignment, so this detects an unwanted final reassignment.
Both the iteration-limit and loose-tolerance stop paths are exercised.

Not executed: deliberate check-failure injection, arithmetic-overflow cases, or
non-finite initializer output (normal DataPoint construction already rejects it).
Previously proposed standalone DataPoint/DataSet checks remain unexecuted.

## Executed during milestone 5

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp KMeansPlusPlusInitialiser.cpp KMeans.cpp main.cpp -o MiniCluster.exe
.\MiniCluster.exe
```

Compilation exited with code 0 and no warnings on the first attempt. Execution
exited with code 0; all earlier demonstrations and checks passed unchanged.
Additional actual output:

```text
PASS: K-Means++ selects two distinct input observations on ordinary data
PASS: K-Means++ accepts k=1
PASS: K-Means++ accepts k=n and returns every unique observation once
PASS: Repeated coordinates preserve multiplicities and select positive weights first
PASS: All-identical data uses zero-weight fallback through k=n
PASS: K-Means++ repeats ordered centres with fresh equal seeds
PASS: K-Means++ rejects empty data, k=0, and k greater than n
PASS: Large finite weights are scaled safely before sampling
PASS: Non-finite squared distances are rejected
PASS: Same fit() accepts RandomInitialiser
Milestone-5 random demo (seed 42):
Iterations: 2; stop: tolerance reached
Cluster 0: centroid (1,2), size 2
Cluster 1: centroid (8,9), size 2
PASS: Same fit() accepts KMeansPlusPlusInitialiser
Milestone-5 K-Means++ demo (seed 42):
Iterations: 2; stop: tolerance reached
Cluster 0: centroid (1,2), size 2
Cluster 1: centroid (8,9), size 2
All milestone-5 checks passed.
```

Eleven new checks use the existing 1e-9 coordinate tolerance and nonzero failure
handling. The repeated-coordinate dataset has two copies of (1,1) and three of
(8,8); k=n checks every observation is represented with its original multiplicity,
and the first two returned coordinates differ while positive weights exist.
The all-identical dataset has four copies of (5,5), also with k=n. Returned copies
cannot reveal which identical index was selected; distinct indices are enforced
by the implementation's selected-index flags, while tests check multiplicities.

The large-weight case uses [-6e153, -6e153, 6e153, 6e153]: individual squared
distances are finite but the unscaled weight sum can overflow. A separate
[-1e308, 1e308] case executes the squared-distance overflow rejection path.
Both strategies' end-to-end results are checked against the expected means and
sizes without assuming cluster order. No test requires different seeds or
different initializer strategies to produce different results.

Not executed: statistical frequency testing of sampling probabilities, deliberate
check-failure injection, or the earlier proposed standalone DataPoint/DataSet
checks. The milestone-4 core arithmetic-overflow paths remain separately untested.

## Executed during milestone 6

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp KMeansPlusPlusInitialiser.cpp KMeans.cpp main.cpp -o MiniCluster.exe
.\MiniCluster.exe
```

The first compilation exited with code 0 and no warnings. Execution exited with
code 0; prior milestone output and checks were preserved. New actual output:

```text
Expected prediction error: Cannot predict before a successful fit.
PASS: Prediction and inertia reject an unfitted model
PASS: Failed first fit leaves the model unfitted
PASS: k=1 gives the overall mean (4.5,5.5)
PASS: One-cluster inertia matches 32.5 + 18.5 + 18.5 + 32.5 = 102
PASS: Two-cluster inertia matches 1 + 1 + 1 + 1 = 4
PASS: predict() assigns obvious nearby points correctly
PASS: Prediction ties choose the lowest cluster index
Expected dimension error: Prediction point dimension must match the fitted model.
PASS: Prediction rejects a dimension mismatch
PASS: Predictions preserve centroids, members, inertia, and stopping metadata
Milestone-6 inertia: k=1 -> 102; k=2 -> 4
Predicted indices: (1,2.2) -> 0; (8,9.2) -> 1
PASS: Rejected refit preserves the successful model and its predictions
PASS: Failure during assignment also preserves the previous fitted model
PASS: Repeated fits replace old data and dimensions without accumulating members
PASS: Early-stop inertia uses reported memberships, not new predictions
PASS: Prediction distance overflow is rejected without changing results
PASS: Inertia sum overflow raises an error while the fitted model remains usable
All milestone-6 checks passed.
```

Fifteen new checks retain the 1e-9 numerical tolerance and nonzero failure handling.
Snapshots compare centroids, member contents/order, inertia, iteration count, and
stopping reason after predictions and failed refits. A replacement dataset has
three 3D points rather than four 2D points; repeated fits retain exactly three
members, means (1,1,1)/(10,10,10), and inertia 6.

Hand calculations: k=1 has mean (4.5,5.5), with squared errors 32.5, 18.5, 18.5,
32.5 summing to 102. The fixed two-cluster result has four squared errors of 1,
summing to 4. After the one-iteration [0,2,3,10] fit, reported members [0] and
[2,3,10] have means 0 and 5, giving inertia 0+9+4+25=38; predict(2) returns 0
without moving that stored member out of cluster 1.

Executed numerical failure paths include overflow during fit assignment, overflow
during prediction, and overflow while summing otherwise finite inertia terms.
The latter uses [-8e153,8e153,-8e153,8e153] with initial mean 0.

Not executed: deliberate self-check failure injection, centroid-update/movement
overflow, allocation failures, or the previously proposed standalone DataPoint/
DataSet edge-case checks. Earlier milestone testing notes are historical records.

## Executed during milestone 7

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp KMeansPlusPlusInitialiser.cpp KMeans.cpp CsvIO.cpp CommandLine.cpp main.cpp -o MiniCluster.exe
python test_csv_cli.py
.\MiniCluster.exe sample.csv 2 kmeans++ 42 100 0.000001 sample-results
```

The first compilation succeeded with no warnings. Both subsequent commands
returned 0. The integration script actually invoked both --self-test and the
no-argument mode and compared their output, preserving all 54 earlier checks.
It uses Python's standard library, creates temporary fixtures, and removes its
own temporary directory automatically. Actual script output:

```text
PASS: --self-test and no arguments preserve all 54 earlier checks
PASS: CLI help
PASS: both initializers export the sample, configuration, inertia, and stopping reason
PASS: blank lines, CRLF, whitespace, scientific notation, 1D/3D, duplicates, k=1/k=n, precision
PASS: 22 invalid CSV cases plus missing-file and directory-input errors
PASS: 15 invalid configuration arguments and wrong argument count
PASS: existing outputs are preserved and unusable output paths fail clearly
PASS: early-stop export uses stored members, not predict() (seed 0)
All milestone-7 integration checks passed.
```

The invalid-file checks include empty/blank-only inputs, headers, leading/trailing/
interior empty fields, inconsistent dimensions, junk suffixes, non-finite values,
overflow/underflow, quotes, alternate delimiters, hexadecimal values, bad signs,
internal whitespace, comments, and BOM. Error messages and failure exit codes are
checked; no outputs may be created for rejected input. Configuration checks cover
invalid counts/methods/seeds/tolerances and argument count. Valid checks read all
three exported files and verify configuration, coordinate multiplicities, cluster
numbering, sizes, means, and inertia. Identical-point k=n covers empty clusters.
Existing-output rejection verifies file contents are unchanged; a file used as
an output parent exercises a filesystem failure. Paths with spaces are tested.

Actual sample-run output:

```text
input_file: sample.csv
points: 4
dimension: 2
k: 2
initialization: kmeans++
seed: 42
maximum_iterations: 100
tolerance: 9.9999999999999995e-07
inertia: 4
iterations: 2
stopping_reason: tolerance_reached
cluster_numbering: zero-based (0 to k-1)
assignment_policy: reported members from the final completed iteration
Cluster 0: centroid (1,2), size 2
Cluster 1: centroid (8,9), size 2
Exported assignments.csv, centroids.csv, summary.txt to sample-results
```

The long tolerance spelling is the round-trip decimal representation of the
double supplied as 0.000001. Inspected assignments.csv:

```csv
feature_1,feature_2,cluster_index
1,1,0
1,3,0
8,8,1
8,10,1
```

Inspected centroids.csv:

```csv
cluster_index,feature_1,feature_2,size
0,1,2,2
1,8,9,2
```

summary.txt contains the first 13 lines of the sample console output. The sample
exports remain available in sample-results/. Use a different directory to rerun.
git diff --check found no whitespace errors (Git printed line-ending notices).

Not executed: actual disk-full/permission-denied/mid-write fault injection,
large-file performance tests, or cross-platform compiler runs. The already
documented unexecuted core edge cases remain outstanding; no extra claims are made.

## Executed during milestone 8

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp KMeansPlusPlusInitialiser.cpp KMeans.cpp CsvIO.cpp CommandLine.cpp main.cpp -o MiniCluster.exe
g++ -std=c++17 -Wall -Wextra -Wpedantic -I. DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp KMeansPlusPlusInitialiser.cpp KMeans.cpp CsvIO.cpp tests/TestSupport.cpp tests/RegressionChecks.cpp tests/TestMain.cpp -o MiniClusterTests.exe
.\MiniClusterTests.exe
python test_csv_cli.py
```

Both first builds exited with code 0 and no warnings. Both suites exited with
code 0. No production defect was demonstrated and no algorithm fixes were made.
The 54 previous regression checks passed in the new executable. Its additional
actual output was:

```text
PASS GROUP: DataPoint validation, ownership, and squared distances
PASS GROUP: DataSet dimensions and unchanged state after rejection
PASS GROUP: Cluster fractional means and empty-cluster policy
PASS GROUP: Both initializers: invalid input and duplicate observations
PASS GROUP: Fixed-centre results independent of label order, k=1, refit, and prediction
PASS GROUP: Lowest-index ties and maximum-iteration termination
Verified 240 fitted partitions: occurrence counts, total sizes, means, and inertia.
PASS GROUP: Duplicate-aware membership and centroid invariants
PASS GROUP: CSV validation in the C++ test executable
All C++ tests passed: 54 regression checks and 8 additional groups.
```

Actual CLI integration output:

```text
PASS: user-facing help and no embedded self-test mode
PASS: both initializers export the sample, configuration, inertia, and stopping reason
PASS: blank lines, CRLF, whitespace, scientific notation, 1D/3D, duplicates, k=1/k=n, precision
PASS: 22 invalid CSV cases plus missing-file and directory-input errors
PASS: 15 invalid configuration arguments and wrong argument count
PASS: existing outputs are preserved and unusable output paths fail clearly
PASS: early-stop export uses stored members, not predict() (seed 0)
All CSV/CLI integration checks passed.
```

### Coverage established by executed checks

| Area | Evidence |
| --- | --- |
| DataPoint | Empty features, NaN, positive/negative infinity rejected; vector copied; dimensions and hand-calculated 1D/2D/3D distances, symmetry/self-distance, mismatches in both directions |
| DataSet | Empty dimension 0; smaller/larger dimensions rejected without changing stored contents; valid insertion still works after rejection |
| Cluster | Fractional 3D means; rejected membership leaves state; empty update and clearing retain previous centroid |
| Input/configuration | Invalid k, empty datasets, bad iteration/tolerance settings, and malformed initializer output |
| Initializers | Both strategies, distinct indices via output multiplicities, duplicate/all-identical data, every valid k, same-seed checks, invalid calls preserve RNG state |
| Expected clustering | Four-point manual example with both starting-centre orders; k=1 overall mean/inertia |
| Stopping/ties | Lowest-index assignment/prediction ties; maximum-iteration and tolerance boundaries; no final reassignment |
| Refit/prediction | Successful replacement without accumulation; dimension changes; rejected first fit/refit; predictions do not mutate results |
| CSV | Valid syntax/blank lines, 22 malformed cases with helpful messages, missing file and directory rejection directly in C++; CLI/export checks separately |
| Partition invariants | 240 fits verify every input occurrence exactly once, total sizes equal n, nonempty centroid means, and independently summed inertia |

The matrix is 5 datasets, both initializers, seeds 0/1/42, every k=1..n, and
iteration limits 1/30. Expected numerical values use tolerances. Returned copied
members use exact coordinate matching against unused input entries: equal rows
count separately, and 0 versus 1e-12 is not merged by tolerance. A deliberately
incorrect synthetic partition with the correct total size is rejected by this
accounting helper. This is a check of the test helper, not an observed model bug.

Clustering comparisons are independent of arbitrary label order. Order-sensitive
legacy assertions are restricted to explicitly fixed starts, defined tie rules,
or preservation/reproducibility of the same model. The reversed-start test proves
the expected-centroid comparison accepts a label permutation.

### Remaining limits

No tests establish correctness for every dataset, global optimality, or statistical
sampling frequencies. Identical points have no stored identity: tests verify
occurrence multiplicities, not an unexposed provenance ID. Large-data performance,
allocation failure, centroid-sum/movement overflow, actual disk-full/permission/
mid-write failures, and cross-platform/library builds remain untested. The runner's
nonzero failure path has not been deliberately triggered. The previously deferred
DataPoint/DataSet validation cases are now covered as listed above.

## Executed during milestone 9

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp KMeansPlusPlusInitialiser.cpp KMeans.cpp CsvIO.cpp CommandLine.cpp main.cpp -o MiniCluster.exe
python experiments/compare_initializers.py --input sample.csv --k 2 --tolerance 0.000001 --max-iterations 100 --seeds 0 1 2 3 4 5 10 20 42 123 --output experiments/results/sample-comparison
python experiments/plot_clusters.py --runs experiments/results/sample-comparison/runs/random/seed-0 experiments/results/sample-comparison/runs/kmeanspp/seed-0 --features 1 2 --output experiments/results/sample-comparison/clusters.png
python experiments/test_experiment.py
```

The application rebuild exited 0 without warnings. Actual comparison output:

```text
Completed 20 C++ runs; results: experiments\results\sample-comparison
random: mean inertia=4, range=[4, 4], mean iterations=2.3, tolerance/limit stops=10/0
kmeans++: mean inertia=4, range=[4, 4], mean iterations=2, tolerance/limit stops=10/0
Paired inertia: random lower=0, kmeans++ lower=0, ties=10
```

Matplotlib was initially missing. The first pip download was blocked by the
sandbox; the approved retry installed it and its dependencies under .plot-deps.
The installer warned about a pre-existing global numba/NumPy version requirement;
numba is not used here and packages were installed in the local target directory.
The first plot invocation could not import Matplotlib correctly because sandbox
access to those installed files was denied. Repeating the same plot command
outside the sandbox succeeded, returning 0:

```text
Saved experiments\results\sample-comparison\clusters.png and experiments\results\sample-comparison\clusters.svg from exported CSV rows.
```

The PNG was visually inspected: both panels show the four exported points, their
cluster colours, labelled black-star centroids, shared axes, method/seed/inertia/
iterations, and the warning that indices need not correspond between methods.

The experiment checks also ran outside the sandbox to access Matplotlib. Actual
output (exit 0):

```text
PASS: all 20 recorded runs match their exported members, means, inertia, and configuration
PASS: per-method statistics recomputed from runs.csv match summary.csv
PASS: generated PNG and SVG artifacts exist
PASS: fresh 3D experiment uses identical settings for both methods
PASS: existing output and duplicate seeds are rejected
PASS: 3D feature projection is labelled and invalid feature numbers are rejected
All experiment checks passed.
```

Checks validate input hashes, the complete method/seed grid, all exported point
multiplicities/means/sizes/inertias, matched configuration, and aggregate metrics.
Four additional C++ runs on a temporary 3D dataset exercised a one-iteration
experiment and features 1/3 projection. No runtime performance or statistical
sampling-frequency conclusions were tested. The main C++ regression suite was
not rerun in this milestone because its code and the core algorithm were unchanged;
the freshly rebuilt application was exercised by 24 actual experiment runs.
Exact commands for a new reproduction directory are in experiments/README.md.
