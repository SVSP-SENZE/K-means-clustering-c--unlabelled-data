# MiniCluster testing

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
