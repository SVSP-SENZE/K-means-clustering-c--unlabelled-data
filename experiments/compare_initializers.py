"""Run the C++ CLI with matched settings; summarize its exported results."""

import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import platform
import statistics
import subprocess


METHODS = ("random", "kmeans++")
DEFAULT_SEEDS = [0, 1, 2, 3, 4, 5, 10, 20, 42, 123]


def read_summary(path):
    return dict(line.split(": ", 1) for line in path.read_text().splitlines())


def write_csv(path, rows):
    with path.open("x", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def summarize(rows):
    summaries = []
    for method in METHODS:
        runs = [row for row in rows if row["method"] == method]
        inertia = [row["inertia"] for row in runs]
        iterations = [row["iterations"] for row in runs]
        summaries.append({
            "method": method, "runs": len(runs),
            "inertia_mean": statistics.mean(inertia),
            "inertia_median": statistics.median(inertia),
            "inertia_min": min(inertia), "inertia_max": max(inertia),
            "inertia_population_stddev": statistics.pstdev(inertia),
            "iterations_mean": statistics.mean(iterations),
            "iterations_min": min(iterations), "iterations_max": max(iterations),
            "tolerance_stops": sum(row["stopping_reason"] == "tolerance_reached" for row in runs),
            "iteration_limit_stops": sum(row["stopping_reason"] == "iteration_limit" for row in runs),
        })
    return summaries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("MiniCluster.exe"))
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--k", type=int, required=True)
    parser.add_argument("--tolerance", type=float, default=1e-6)
    parser.add_argument("--max-iterations", type=int, default=100)
    parser.add_argument("--seeds", type=int, nargs="+", default=DEFAULT_SEEDS)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.k < 1 or args.max_iterations < 1 or not math.isfinite(args.tolerance) or args.tolerance < 0:
        parser.error("k/maximum iterations must be positive and tolerance finite/nonnegative")
    if len(set(args.seeds)) != len(args.seeds) or any(not 0 <= seed <= 4294967295 for seed in args.seeds):
        parser.error("seeds must be distinct integers in [0, 4294967295]")
    exe = args.exe.resolve()
    if not exe.is_file() or not args.input.is_file():
        parser.error("the executable and input file must exist")
    root = args.output.resolve()
    if root.exists():
        parser.error("output directory already exists; choose a new directory")
    input_bytes = args.input.read_bytes()
    root.mkdir(parents=True)
    snapshot = root / "input.csv"
    snapshot.write_bytes(input_bytes)
    configuration = {
        "original_input": str(args.input.resolve()),
        "input_sha256": hashlib.sha256(input_bytes).hexdigest(),
        "executable_sha256": hashlib.sha256(exe.read_bytes()).hexdigest(),
        "platform": platform.platform(), "python": platform.python_version(),
        "k": args.k, "tolerance": args.tolerance, "maximum_iterations": args.max_iterations,
        "methods": list(METHODS), "seeds": args.seeds,
        "plot_seed_selected_in_advance": args.seeds[0],
    }
    (root / "configuration.json").write_text(json.dumps(configuration, indent=2) + "\n")
    rows = []
    dataset_shape = None
    for method in METHODS:
        for seed in args.seeds:
            relative_run = Path("runs") / ("random" if method == "random" else "kmeanspp") / f"seed-{seed}"
            run_directory = root / relative_run
            command = [str(exe), str(snapshot), str(args.k), method, str(seed),
                       str(args.max_iterations), repr(args.tolerance), str(run_directory)]
            result = subprocess.run(command, capture_output=True, text=True, timeout=300)
            if result.returncode != 0:
                raise RuntimeError(f"C++ run failed ({method}, seed {seed}): {result.stderr.strip()}")
            summary = read_summary(run_directory / "summary.txt")
            if (summary["initialization"] != method or int(summary["seed"]) != seed
                    or int(summary["k"]) != args.k
                    or int(summary["maximum_iterations"]) != args.max_iterations
                    or float(summary["tolerance"]) != args.tolerance):
                raise RuntimeError("Exported configuration differs from the requested experiment")
            shape = (int(summary["points"]), int(summary["dimension"]))
            if dataset_shape is not None and shape != dataset_shape:
                raise RuntimeError("Dataset shape differs between runs")
            dataset_shape = shape
            inertia = float(summary["inertia"])
            iterations = int(summary["iterations"])
            reason = summary["stopping_reason"]
            if not math.isfinite(inertia) or inertia < 0 or not 1 <= iterations <= args.max_iterations:
                raise RuntimeError("Invalid exported inertia or iteration count")
            if reason not in ("tolerance_reached", "iteration_limit"):
                raise RuntimeError("Unknown stopping reason")
            rows.append({"method": method, "seed": seed, "inertia": inertia,
                         "iterations": iterations, "stopping_reason": reason,
                         "run_directory": relative_run.as_posix()})
    write_csv(root / "runs.csv", rows)
    summaries = summarize(rows)
    write_csv(root / "summary.csv", summaries)

    paired = {"random": 0, "kmeans++": 0, "ties": 0}
    for seed in args.seeds:
        values = {row["method"]: row["inertia"] for row in rows if row["seed"] == seed}
        if math.isclose(values["random"], values["kmeans++"], rel_tol=1e-9, abs_tol=1e-9):
            paired["ties"] += 1
        elif values["random"] < values["kmeans++"]:
            paired["random"] += 1
        else:
            paired["kmeans++"] += 1
    report = ["# Initialization experiment", "",
              f"Input: {args.input.name}; {dataset_shape[0]} points, {dataset_shape[1]} dimensions.",
              f"k={args.k}; tolerance={args.tolerance}; maximum iterations={args.max_iterations}.",
              "Seeds (same ordered list for both methods): " + ", ".join(map(str, args.seeds)) + ".", "",
              "| Method | Runs | Mean inertia | Min / max inertia | Population SD | Mean iterations | Min / max iterations | Tolerance / limit stops |",
              "| --- | ---: | ---: | --- | ---: | ---: | --- | --- |"]
    for row in summaries:
        report.append(f"| {row['method']} | {row['runs']} | {row['inertia_mean']:.6g} | "
                      f"{row['inertia_min']:.6g} / {row['inertia_max']:.6g} | {row['inertia_population_stddev']:.6g} | "
                      f"{row['iterations_mean']:.6g} | {row['iterations_min']} / {row['iterations_max']} | "
                      f"{row['tolerance_stops']} / {row['iteration_limit_stops']} |")
    report += ["", f"Paired lower-inertia counts: random={paired['random']}, kmeans++={paired['kmeans++']}, ties={paired['ties']}.",
               "Ties use absolute and relative tolerance 1e-9. Iteration counts are not wall-clock timings.",
               "Statistics are descriptive over this seed list (population standard deviation), not significance tests.",
               "Both methods receive the same seed values but consume random numbers differently.",
               "This dataset/seed comparison does not establish that k-means++ always wins or that either method finds a global optimum.",
               "Inertia uses exported fitted memberships, including any iteration-limit stops.",
               f"The plot uses seed {args.seeds[0]}, the first listed seed, chosen before observing results.",
               "All fitting/assignment/centroid calculations run in C++; Python invokes the executable and summarizes exports.",
               "For higher-dimensional data, a plot of two selected features is only a projection: hidden dimensions still influence fitting.", ""]
    (root / "report.md").write_text("\n".join(report), encoding="utf-8")
    print(f"Completed {len(rows)} C++ runs; results: {args.output}")
    for row in summaries:
        print(f"{row['method']}: mean inertia={row['inertia_mean']:.6g}, "
              f"range=[{row['inertia_min']:.6g}, {row['inertia_max']:.6g}], "
              f"mean iterations={row['iterations_mean']:.6g}, "
              f"tolerance/limit stops={row['tolerance_stops']}/{row['iteration_limit_stops']}")
    print(f"Paired inertia: random lower={paired['random']}, kmeans++ lower={paired['kmeans++']}, ties={paired['ties']}")


if __name__ == "__main__":
    main()
