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
