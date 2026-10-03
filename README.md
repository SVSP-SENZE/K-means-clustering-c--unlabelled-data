# MiniCluster

C++17 K-means for arbitrary-dimensional numerical points. Seven domain classes
handle points, datasets, clusters, fitting/prediction, and two initialization
strategies. CSV and command-line handling use separate free functions, not new
domain classes.

## Build and run (PowerShell)

From this directory, with g++ on PATH:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp KMeansPlusPlusInitialiser.cpp KMeans.cpp CsvIO.cpp CommandLine.cpp main.cpp -o MiniCluster.exe
.\MiniCluster.exe sample.csv 2 kmeans++ 42 100 0.000001 my-results
```

Use a **new output directory** for each run. To compare random initialization:

```powershell
.\MiniCluster.exe sample.csv 2 random 42 100 0.000001 random-results
.\MiniCluster.exe --help
```

sample-results/ contains the actual example export generated during milestone 7.
Use my-results (or another new name) to run it yourself; choose a new name again
on subsequent runs.

The positional arguments, in order, are:

| Argument | Meaning |
| --- | --- |
| INPUT.csv | Input filename; quote paths containing spaces |
| K | Positive number of clusters, at most the point count |
| random or kmeans++ | Exact initialization method name |
| SEED | Unsigned decimal integer from 0 through 4294967295 |
| MAX_ITERATIONS | Positive integer |
| TOLERANCE | Finite, nonnegative Euclidean centroid-movement tolerance |
| OUTPUT_DIR | Directory to create for this run; must not already exist |

K and MAX_ITERATIONS must fit std::size_t. Integer arguments contain digits only.
No optional defaults are hidden: all seven arguments are required for a CSV run.
No arguments or --help displays usage. Run MiniClusterTests.exe for tests;
the application no longer has an embedded --self-test mode.
Errors print ERROR with a useful explanation and return exit code 1; success is 0.

## Strict input format

- No header: one observation per nonblank row, comma-separated numerical fields.
- The first data row establishes a positive dimension; every data row must match.
- ASCII numeric text or UTF-8 without a byte-order mark (BOM); LF and CRLF endings
  are supported. A final newline is optional.
- Decimal syntax allows an optional sign, a decimal point, and an optional e/E
  exponent, for example 1, -2.5, +.5, 3., or 1e-3. At least one digit is required.
  Conversion must consume the whole field and produce a finite double. Values
  outside double's conversion range, including underflow, are rejected.
- Spaces and tabs around fields are allowed. Internal whitespace is rejected.
- Empty or spaces/tabs-only lines are ignored, including leading/trailing lines.
  Blank lines do not count as points; error line numbers still count physical lines.
- Empty fields (including trailing commas), quoted fields, headers, comments,
  NaN/infinity, hexadecimal numbers, and alternate separators are rejected.
- Missing/unreadable files, directories, and datasets with no data rows are errors.
  Field errors identify the filename, line, and field; dimension errors identify
  the line and expected/actual field counts.

Included sample.csv:

```text
1,1
1,3
8,8
8,10
```

## Export format and numbering

Each successful CSV run creates three files:

- assignments.csv: header feature_1,...,feature_D,cluster_index followed by one
  row per fitted member. Rows are grouped by cluster index, with member order
  preserved inside each cluster. Original global input order is not retained;
  duplicate observations are all exported.
- centroids.csv: header cluster_index,feature_1,...,feature_D,size followed by
  one row for every cluster, including empty clusters.
- summary.txt: input filename, point count, dimension, k, initialization method,
  seed, maximum iterations, tolerance, inertia, completed iteration count,
  stopping reason, numbering, and assignment policy.

Cluster indices are **zero-based, 0 through k-1** throughout both CSV files,
console output, and predict(). They identify groups, not semantic class labels,
and can change order between runs. Feature column names start at feature_1.

Assignments come from stored fitted members, not a new predict() pass. Centroids,
memberships, and inertia describe the same completed assignment/update iteration.
A tolerance or iteration-limit stop does not guarantee an assignment fixed point.

Exports have headers and metadata columns and are not directly valid input files.
Output numbers use a dot decimal separator and max_digits10 precision so doubles
can round-trip. The output directory must be new; existing outputs are never
overwritten. Output errors return failure. A disk/write failure can leave partial
files in the new directory; exports are not a multi-file atomic transaction.

## Checks

Build the separate C++ test executable (no external framework):

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -I. DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp KMeansPlusPlusInitialiser.cpp KMeans.cpp CsvIO.cpp tests/TestSupport.cpp tests/RegressionChecks.cpp tests/TestMain.cpp -o MiniClusterTests.exe
.\MiniClusterTests.exe
python test_csv_cli.py
```

Build the application using the command above before running the Python script.
The C++ suite preserves the 54 earlier regression checks and adds eight groups,
including direct CSV validation and 240 fitted-result invariant checks. Test-only
helpers and the fixed initializer live under tests/; main.cpp only launches the
user-facing CLI. -I. lets test sources find headers in the project root.

The Python script is an additional CLI/export integration check, requiring only
Python 3's standard library. It is not required to build or run the C++ suite.
Both suites use temporary CSV fixtures, report failures, and return nonzero on
failure. See TESTING.md for coverage, executed results, and remaining limitations.
