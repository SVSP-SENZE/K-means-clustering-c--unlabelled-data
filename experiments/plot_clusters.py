"""Plot exported C++ memberships and centroids; never recompute clustering."""

import argparse
import csv
import os
from pathlib import Path
import sys


PROJECT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJECT / ".plot-deps"))
os.environ.setdefault("MPLCONFIGDIR", str(PROJECT / ".mpl-cache"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runs", type=Path, nargs="+", required=True, help="One or two exported run directories")
    parser.add_argument("--features", type=int, nargs=2, default=[1, 2], help="One-based feature column numbers")
    parser.add_argument("--output", type=Path, required=True, help="New PNG filename; SVG is also written")
    args = parser.parse_args()
    if not 1 <= len(args.runs) <= 2 or min(args.features) < 1 or len(set(args.features)) != 2:
        parser.error("use one/two run directories and two distinct positive feature numbers")
    if args.output.suffix.lower() != ".png":
        parser.error("output must end in .png")
    svg = args.output.with_suffix(".svg")
    if args.output.exists() or svg.exists():
        parser.error("plot output already exists; choose a new name")
    records = []
    for directory in args.runs:
        summary = dict(line.split(": ", 1) for line in (directory / "summary.txt").read_text().splitlines())
        dimension = int(summary["dimension"])
        if max(args.features) > dimension:
            parser.error(f"selected feature does not exist in {directory} (dimension {dimension})")
        with (directory / "assignments.csv").open(newline="") as stream:
            points = list(csv.DictReader(stream))
        with (directory / "centroids.csv").open(newline="") as stream:
            centres = list(csv.DictReader(stream))
        if len(points) != int(summary["points"]) or len(centres) != int(summary["k"]):
            parser.error(f"export row counts disagree with summary in {directory}")
        records.append((summary, points, centres))

    import matplotlib
    matplotlib.use("Agg")  # Save files without opening a GUI window.
    import matplotlib.pyplot as plt

    xkey, ykey = (f"feature_{i}" for i in args.features)
    fig, axes = plt.subplots(1, len(records), figsize=(6 * len(records), 5.5),
                             squeeze=False, sharex=True, sharey=True)
    colours = plt.get_cmap("tab10")
    for ax, (summary, points, centres) in zip(axes[0], records):
        for centre in centres:
            index = int(centre["cluster_index"])
            members = [p for p in points if int(p["cluster_index"]) == index]
            ax.scatter([float(p[xkey]) for p in members], [float(p[ykey]) for p in members],
                       s=75, color=colours(index % 10), edgecolors="white", linewidths=0.8,
                       label=f"Cluster {index} (n={len(members)})", zorder=3)
            x, y = float(centre[xkey]), float(centre[ykey])
            ax.scatter([x], [y], s=240, marker="*", color="black", edgecolors="white",
                       linewidths=0.8, label="Centroid" if index == 0 else None, zorder=4)
            ax.annotate(f"C{index}", (x, y), xytext=(10, 5), textcoords="offset points", weight="bold")
        method = "Random initialization" if summary["initialization"] == "random" else "K-Means++ initialization"
        ax.set_title(f"{method}\nSeed {summary['seed']} | inertia {float(summary['inertia']):.6g} | "
                     f"iterations {summary['iterations']}", fontsize=12)
        ax.set_xlabel(f"Feature {args.features[0]}")
        ax.set_ylabel(f"Feature {args.features[1]}")
        ax.set_aspect("equal", adjustable="box")
        ax.grid(alpha=0.2, zorder=0)
        ax.margins(0.2)
        ax.legend(loc="upper left", fontsize=9)
    projection = any(int(summary["dimension"]) > 2 for summary, _, _ in records)
    title = "Cluster memberships and exported centroids" + (" — 2D projection" if projection else "")
    fig.suptitle(title, fontsize=16, weight="bold")
    note = "Colours identify cluster indices within each run; labels need not correspond across methods."
    if projection:
        note += "\nOnly two features are shown; hidden dimensions still determine clustering."
    fig.text(0.5, 0.025, note, ha="center", fontsize=9)
    fig.tight_layout(rect=(0, 0.08, 1, 0.93))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=180)
    fig.savefig(svg)
    plt.close(fig)
    print(f"Saved {args.output} and {svg} from exported CSV rows.")
    if projection:
        print("This is a projection, not a complete picture of the fitted dimensions.")


if __name__ == "__main__":
    main()
