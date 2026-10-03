"""Integration checks using only Python's standard library and the built executable."""

import collections
import csv
import math
from pathlib import Path
import subprocess
import tempfile


EXE = Path(__file__).resolve().parent / "MiniCluster.exe"


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def invoke(*arguments):
    return subprocess.run([str(EXE), *map(str, arguments)], capture_output=True,
                          text=True, timeout=20)


def read_exports(directory, expected_points):
    with (directory / "assignments.csv").open(newline="") as stream:
        assigned = list(csv.DictReader(stream))
    with (directory / "centroids.csv").open(newline="") as stream:
        centres = list(csv.DictReader(stream))
    summary = dict(line.split(": ", 1) for line in
                   (directory / "summary.txt").read_text().splitlines())
    dimension = len(expected_points[0])
    features = [f"feature_{i + 1}" for i in range(dimension)]
    require(list(assigned[0]) == features + ["cluster_index"], "Assignment headers")
    require(list(centres[0]) == ["cluster_index", *features, "size"], "Centroid headers")
    points = [tuple(float(row[key]) for key in features) for row in assigned]
    require(collections.Counter(points) == collections.Counter(expected_points),
            "Every input observation must be exported once, including duplicates")
    require(int(summary["points"]) == len(points), "Summary point count")
    require(int(summary["dimension"]) == dimension, "Summary dimension")
    require(int(summary["k"]) == len(centres), "Summary k")
    require([int(row["cluster_index"]) for row in centres] == list(range(len(centres))),
            "Centroid indices must be zero-based and consecutive")
    inertia = 0.0
    for index, centre in enumerate(centres):
        members = [p for p, row in zip(points, assigned) if int(row["cluster_index"]) == index]
        require(len(members) == int(centre["size"]), "Exported cluster size")
        mean = [float(centre[key]) for key in features]
        if members:
            for j in range(dimension):
                require(math.isclose(mean[j], sum(p[j] for p in members) / len(members),
                                     rel_tol=1e-12, abs_tol=1e-12), "Centroid/member consistency")
        inertia += sum(sum((p[j] - mean[j]) ** 2 for j in range(dimension)) for p in members)
    require(all(0 <= int(row["cluster_index"]) < len(centres) for row in assigned),
            "Assignment index range")
    require(math.isclose(inertia, float(summary["inertia"]), rel_tol=1e-12, abs_tol=1e-12),
            "Summary inertia must use exported memberships")
    return assigned, centres, summary


def main():
    default = invoke()
    help_result = invoke("--help")
    require(default.returncode == 0 and help_result.returncode == 0
            and default.stdout == help_result.stdout and "Usage:" in default.stdout
            and "PASS:" not in default.stdout, "User-facing help without test execution")
    require(invoke("--self-test").returncode == 1, "Tests have a separate executable")
    print("PASS: user-facing help and no embedded self-test mode")

    with tempfile.TemporaryDirectory(prefix="minicluster-csv-") as temporary:
        root = Path(temporary)
        input_path = root / "points with spaces.csv"
        input_path.write_text("1,1\n1,3\n8,8\n8,10\n", encoding="utf-8")
        expected = [(1.0, 1.0), (1.0, 3.0), (8.0, 8.0), (8.0, 10.0)]
        for method in ("random", "kmeans++"):
            output = root / method
            result = invoke(input_path, 2, method, 42, 100, "1e-6", output)
            require(result.returncode == 0, result.stderr)
            _, centres, summary = read_exports(output, expected)
            require({(float(c["feature_1"]), float(c["feature_2"])) for c in centres}
                    == {(1.0, 2.0), (8.0, 9.0)}, "Expected sample centroids")
            require(summary["initialization"] == method and summary["seed"] == "42"
                    and summary["maximum_iterations"] == "100"
                    and float(summary["tolerance"]) == 1e-6
                    and summary["stopping_reason"] == "tolerance_reached"
                    and summary["iterations"] == "2", "Summary configuration/results")
        print("PASS: both initializers export the sample, configuration, inertia, and stopping reason")

        valid = [
            ("\r\n \t\r\n +1e0, .5, -2.\r\n\t3 , 1.5, +4E0\r\n\r\n",
             [(1.0, 0.5, -2.0), (3.0, 1.5, 4.0)], 1),
            ("5\n5\n5", [(5.0,), (5.0,), (5.0,)], 3),
            ("0.12345678901234566\n0.9876543210987654\n",
             [(0.12345678901234566,), (0.9876543210987654,)], 1),
        ]
        for index, (content, points, k) in enumerate(valid):
            path = root / f"valid-{index}.csv"
            path.write_bytes(content.encode("utf-8"))
            output = root / f"valid-{index}-out"
            result = invoke(path, k, "kmeans++", 0, 10, 0, output)
            require(result.returncode == 0, result.stderr)
            read_exports(output, points)
        print("PASS: blank lines, CRLF, whitespace, scientific notation, 1D/3D, duplicates, k=1/k=n, precision")

        invalid = [
            ("", "no data points"), ("\n \t\r\n", "no data points"),
            ("x,y\n", "line 1, field 1"), ("1,\n", "line 1, field 2: empty field"),
            (",2\n", "field 1: empty field"), ("1,,2\n", "field 2: empty field"),
            ("1, \t\n", "field 2: empty field"), ("1,2\n3\n", "line 2: expected 2 fields, got 1"),
            ("\n1,2\n3,4,5\n", "line 3: expected 2 fields, got 3"),
            ("1,2oops\n", "field 2"), ("1,NaN\n", "finite decimal"),
            ("1,inf\n", "finite decimal"), ("1,-Infinity\n", "finite decimal"),
            ("1,1e9999\n", "outside the double range"), ("1,1e-9999\n", "outside the double range"),
            ('"1",2\n', "field 1"), ("1;2\n", "field 1"),
            ("0x1p2,2\n", "field 1"), ("+-1,2\n", "invalid number"),
            ("1 2,3\n", "field 1"), ("1,2 #comment\n", "field 2"),
            ("\ufeff1,2\n", "field 1"),
        ]
        for index, (content, message) in enumerate(invalid):
            path = root / f"invalid-{index}.csv"
            path.write_bytes(content.encode("utf-8"))
            output = root / f"invalid-{index}-out"
            result = invoke(path, 1, "random", 42, 10, 0, output)
            require(result.returncode == 1 and message in result.stderr,
                    f"Invalid CSV {index}: {result.stdout} {result.stderr}")
            require(not output.exists(), "Invalid input must not create outputs")
        missing = invoke(root / "missing.csv", 1, "random", 42, 10, 0, root / "missing-out")
        directory_input = invoke(root, 1, "random", 42, 10, 0, root / "directory-out")
        require(missing.returncode == 1 and "missing" in missing.stderr, "Missing file error")
        require(directory_input.returncode == 1 and "regular file" in directory_input.stderr,
                "Directory is not a CSV file")
        print("PASS: 22 invalid CSV cases plus missing-file and directory-input errors")

        arguments = [str(input_path), "2", "random", "42", "100", "1e-6", str(root / "bad-cli")]
        invalid_arguments = [(1, "0"), (1, "-1"), (1, "2x"), (1, "1.5"), (1, "5"),
                             (1, "999999999999999999999999999999"), (2, "other"),
                             (3, "-1"), (3, "4294967296"), (4, "0"), (4, "-3"),
                             (5, "-0.1"), (5, "nan"), (5, "1e9999"), (5, "1junk")]
        for index, value in invalid_arguments:
            changed = arguments.copy()
            changed[index] = value
            result = invoke(*changed)
            require(result.returncode == 1 and "ERROR:" in result.stderr,
                    f"Invalid CLI: {changed}: {result.stderr}")
        require(invoke("--unknown").returncode == 1, "Unknown option/argument count")
        print("PASS: 15 invalid configuration arguments and wrong argument count")

        existing = root / "random"
        before = {p.name: p.read_bytes() for p in existing.iterdir()}
        refused = invoke(input_path, 2, "random", 42, 100, 0, existing)
        require(refused.returncode == 1 and "already exists" in refused.stderr,
                "Existing output must be rejected")
        require(before == {p.name: p.read_bytes() for p in existing.iterdir()}, "No output overwrite")
        blocked_parent = root / "not-a-directory"
        blocked_parent.write_text("keep this", encoding="utf-8")
        failed_write = invoke(input_path, 2, "random", 42, 100, 0, blocked_parent / "out")
        require(failed_write.returncode == 1 and "ERROR:" in failed_write.stderr, "Output path error")
        require(blocked_parent.read_text() == "keep this", "Output failure preserves existing file")
        print("PASS: existing outputs are preserved and unusable output paths fail clearly")

        early = root / "early.csv"
        early.write_text("0\n2\n3\n10\n", encoding="utf-8")
        found = False
        for seed in range(32):
            output = root / f"early-{seed}"
            result = invoke(early, 2, "random", seed, 1, 0, output)
            require(result.returncode == 0, result.stderr)
            assigned, centres, summary = read_exports(output, [(0.0,), (2.0,), (3.0,), (10.0,)])
            for row in assigned:
                point = float(row["feature_1"])
                nearest = min(range(2), key=lambda i: (point - float(centres[i]["feature_1"])) ** 2)
                if nearest != int(row["cluster_index"]):
                    found = True
            if found:
                require(summary["stopping_reason"] == "iteration_limit", "Early-stop reason")
                break
        require(found, "Exercise a reported member differing from fresh prediction")
        print(f"PASS: early-stop export uses stored members, not predict() (seed {seed})")
    print("All CSV/CLI integration checks passed.")


if __name__ == "__main__":
    main()
