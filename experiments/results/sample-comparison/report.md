# Initialization experiment

Input: sample.csv; 4 points, 2 dimensions.
k=2; tolerance=1e-06; maximum iterations=100.
Seeds (same ordered list for both methods): 0, 1, 2, 3, 4, 5, 10, 20, 42, 123.

| Method | Runs | Mean inertia | Min / max inertia | Population SD | Mean iterations | Min / max iterations | Tolerance / limit stops |
| --- | ---: | ---: | --- | ---: | ---: | --- | --- |
| random | 10 | 4 | 4 / 4 | 0 | 2.3 | 2 / 3 | 10 / 0 |
| kmeans++ | 10 | 4 | 4 / 4 | 0 | 2 | 2 / 2 | 10 / 0 |

Paired lower-inertia counts: random=0, kmeans++=0, ties=10.
Ties use absolute and relative tolerance 1e-9. Iteration counts are not wall-clock timings.
Statistics are descriptive over this seed list (population standard deviation), not significance tests.
Both methods receive the same seed values but consume random numbers differently.
This dataset/seed comparison does not establish that k-means++ always wins or that either method finds a global optimum.
Inertia uses exported fitted memberships, including any iteration-limit stops.
The plot uses seed 0, the first listed seed, chosen before observing results.
All fitting/assignment/centroid calculations run in C++; Python invokes the executable and summarizes exports.
For higher-dimensional data, a plot of two selected features is only a projection: hidden dimensions still influence fitting.
