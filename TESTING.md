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
