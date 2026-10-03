"""Check recorded exports and the experiment/projection workflow using real C++ runs."""

import csv
import hashlib
import json
import math
from pathlib import Path
import statistics
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))
from test_csv_cli import read_exports, require


def read_rows(path):
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream))


def verify(directory):
    config = json.loads((directory / "configuration.json").read_text())
    raw = (directory / "input.csv").read_bytes()
    require(hashlib.sha256(raw).hexdigest() == config["input_sha256"], "Input snapshot hash")
    points = [tuple(map(float, row)) for row in csv.reader(raw.decode().splitlines()) if row]
    runs = read_rows(directory / "runs.csv")
    require(len(runs) == len(config["seeds"]) * 2, "Run count")
    require({(r["method"], int(r["seed"])) for r in runs}
            == {(method, seed) for method in config["methods"] for seed in config["seeds"]},
            "Exactly one run per method/seed")
    for row in runs:
        _, _, summary = read_exports(directory / row["run_directory"], points)
        require(row["method"] == summary["initialization"] and row["seed"] == summary["seed"], "Run identity")
        require(float(row["inertia"]) == float(summary["inertia"])
                and row["iterations"] == summary["iterations"]
                and row["stopping_reason"] == summary["stopping_reason"], "Recorded exported metrics")
        require(int(summary["k"]) == config["k"]
                and float(summary["tolerance"]) == config["tolerance"]
                and int(summary["maximum_iterations"]) == config["maximum_iterations"], "Matched settings")
    for row in read_rows(directory / "summary.csv"):
        matching = [r for r in runs if r["method"] == row["method"]]
        inertia = [float(r["inertia"]) for r in matching]
        iterations = [int(r["iterations"]) for r in matching]
        require(int(row["runs"]) == len(matching), "Aggregate run count")
        expected = {"inertia_mean": statistics.mean(inertia), "inertia_median": statistics.median(inertia),
                    "inertia_min": min(inertia), "inertia_max": max(inertia),
                    "inertia_population_stddev": statistics.pstdev(inertia),
                    "iterations_mean": statistics.mean(iterations), "iterations_min": min(iterations),
                    "iterations_max": max(iterations),
                    "tolerance_stops": sum(r["stopping_reason"] == "tolerance_reached" for r in matching),
                    "iteration_limit_stops": sum(r["stopping_reason"] == "iteration_limit" for r in matching)}
        for field, value in expected.items():
            require(math.isclose(float(row[field]), value, rel_tol=1e-12, abs_tol=1e-12), field)
    return len(runs)


def run(script, *arguments):
    return subprocess.run([sys.executable, str(ROOT / "experiments" / script), *map(str, arguments)],
                          capture_output=True, text=True, timeout=60, cwd=ROOT)


def main():
    published = ROOT / "experiments/results/sample-comparison"
    count = verify(published)
    require((published / "clusters.png").read_bytes().startswith(b"\x89PNG\r\n\x1a\n"), "PNG artifact")
    require("<svg" in (published / "clusters.svg").read_text(), "SVG artifact")
    print(f"PASS: all {count} recorded runs match their exported members, means, inertia, and configuration")
    print("PASS: per-method statistics recomputed from runs.csv match summary.csv")
    print("PASS: generated PNG and SVG artifacts exist")
    with tempfile.TemporaryDirectory(prefix="minicluster-experiment-") as temporary:
        folder = Path(temporary)
        input_file = folder / "3d.csv"
        input_file.write_text("1,2,3\n3,4,5\n9,8,7\n")
        output = folder / "experiment"
        command = ["--exe", ROOT / "MiniCluster.exe", "--input", input_file, "--k", 2,
                   "--max-iterations", 1, "--tolerance", 0, "--seeds", 0, 42, "--output", output]
        result = run("compare_initializers.py", *command)
        require(result.returncode == 0, result.stderr)
        require(verify(output) == 4, "3D experiment with iteration-limit configuration")
        print("PASS: fresh 3D experiment uses identical settings for both methods")
        again = run("compare_initializers.py", *command)
        require(again.returncode != 0 and "already exists" in again.stderr, "Refuse overwrite")
        bad_seeds = run("compare_initializers.py", "--input", input_file, "--k", 2,
                        "--seeds", 0, 0, "--output", folder / "bad")
        require(bad_seeds.returncode != 0 and "distinct" in bad_seeds.stderr, "Reject duplicate seeds")
        print("PASS: existing output and duplicate seeds are rejected")
        plot = run("plot_clusters.py", "--runs", output / "runs/random/seed-0",
                   output / "runs/kmeanspp/seed-0", "--features", 1, 3, "--output", folder / "projection.png")
        require(plot.returncode == 0 and "projection" in plot.stdout, plot.stderr)
        require("2D projection" in (folder / "projection.svg").read_text(), "Projection label")
        invalid_plot = run("plot_clusters.py", "--runs", output / "runs/random/seed-0",
                           "--features", 1, 4, "--output", folder / "invalid.png")
        require(invalid_plot.returncode != 0 and "does not exist" in invalid_plot.stderr, "Feature bounds")
        print("PASS: 3D feature projection is labelled and invalid feature numbers are rejected")
    print("All experiment checks passed.")


if __name__ == "__main__":
    main()
