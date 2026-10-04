# Initialization experiment

The runner invokes MiniCluster.exe; **all clustering remains in C++**. Python
only launches matched runs, reads their exports, summarizes metrics, and plots.
The core classes and KMeans loop are unchanged. These optional experiment/plot
utilities are separate from the C++ application and test suite.

View the figures in the [main README graph gallery](../README.md#graph-gallery).

## Recorded experiment

Dataset: the existing sample.csv, four 2D observations (1,1), (1,3), (8,8), (8,10).
Both methods use k=2, tolerance=0.000001, maximum iterations=100, and seeds:

```text
0, 1, 2, 3, 4, 5, 10, 20, 42, 123
```

These settings and the plotted seed 0 were chosen before running the comparison.
No result was discarded. Each method has ten runs (twenty total). A single input
snapshot is used for every run. configuration.json records its SHA-256 hash,
the executable hash, platform, parameters, and seeds.

| Method | Mean inertia | Min / max inertia | Population SD | Mean iterations | Min / max iterations | Tolerance / limit stops |
| --- | ---: | --- | ---: | ---: | --- | --- |
| Random | 4 | 4 / 4 | 0 | 2.3 | 2 / 3 | 10 / 0 |
| K-Means++ | 4 | 4 / 4 | 0 | 2.0 | 2 / 2 | 10 / 0 |

All ten seed pairs tie on inertia. Random takes three iterations at seeds 1, 4,
and 20, and two otherwise. K-Means++ takes two in every recorded run. This is
descriptive evidence about this tiny, easy dataset only. It does not show that
K-Means++ always wins, reaches a global optimum, or runs faster: initialization
costs differ, and no wall-clock benchmark was performed. Same seeds do not imply
the same random draws across different initialization procedures.

## Files

Under results/sample-comparison/:

- runs.csv: method, seed, final inertia, iterations, stopping reason, export path.
- summary.csv: per-method count, inertia mean/median/min/max/population standard
  deviation, iteration mean/min/max, and counts of each stopping reason.
- report.md: generated description, table, and paired lower-inertia/tie counts.
- configuration.json and input.csv: configuration/provenance and dataset snapshot.
- runs/random/seed-N and runs/kmeanspp/seed-N: actual C++ assignments.csv,
  centroids.csv, and summary.txt for every run.
- clusters.png and clusters.svg: seed-0 memberships coloured by cluster, with
  black star centroids labelled C0/C1; both panels use the same axes.

The scripts refuse to overwrite existing experiment/plot outputs. Failed runs
raise errors instead of being silently omitted; partial output may remain.
Aggregates are built from summary.txt exports, not scraped console messages.

## Exact reproduction commands (PowerShell, project root)

Compile the application:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude DataPoint.cpp DataSet.cpp Cluster.cpp RandomInitialiser.cpp KMeansPlusPlusInitialiser.cpp KMeans.cpp CsvIO.cpp JsonIO.cpp SvgPlot.cpp CommandLine.cpp main.cpp -o MiniCluster.exe
```

Run the comparison into a new directory (the recorded sample-comparison exists):

```powershell
python experiments/compare_initializers.py --input sample.csv --k 2 --tolerance 0.000001 --max-iterations 100 --seeds 0 1 2 3 4 5 10 20 42 123 --output experiments/results/reproduction
```

The experiment runner needs only Python's standard library. For plotting, install
the recorded Matplotlib dependencies locally (tested with Python 3.14.4, Windows):

```powershell
python -m pip install --target .plot-deps -r experiments/plot-requirements.txt
python experiments/plot_clusters.py --runs experiments/results/reproduction/runs/random/seed-0 experiments/results/reproduction/runs/kmeanspp/seed-0 --features 1 2 --output experiments/results/reproduction/clusters.png
```

.plot-deps and the Matplotlib cache are ignored by Git. Plotting uses the local
dependency directory automatically, and never fits or reassigns observations.
Matplotlib's Agg backend writes PNG/SVG without opening an application window.
Random-distribution implementations can vary across C++ standard libraries, so
identical seeds are not a promise of identical results across toolchains.

Validate the recorded results and a fresh temporary 3D experiment:

```powershell
python experiments/test_experiment.py
```

## Higher-dimensional data

--features uses one-based feature column numbers, for example --features 1 3.
For dimension >2, the title explicitly says **2D projection**: only the selected
coordinates are drawn, while fitting and inertia still use every dimension.
Overlap in the plot need not mean overlap in the full feature space. For 1D input,
two distinct features do not exist and this scatter-plot command rejects it.

Cluster numbers and colours are local to each run, not semantic labels. For seed
0 here, the two methods have reversed cluster indices despite the same grouping.
Reported memberships are plotted directly; early-stopped results are not silently
reassigned. Duplicate observations may overlap visually; legend counts retain
their multiplicities.
