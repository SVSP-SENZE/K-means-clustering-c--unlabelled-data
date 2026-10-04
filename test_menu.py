"""End-to-end checks of the guided menu using temporary output directories."""
from pathlib import Path
import json
import subprocess
import tempfile

EXE = Path(__file__).resolve().parent / "MiniCluster.exe"

def run(text, cwd):
    result = subprocess.run([str(EXE)], input=text, text=True, capture_output=True,
                            cwd=cwd, timeout=20)
    assert result.returncode == 0, result.stderr
    return result.stdout

with tempfile.TemporaryDirectory(prefix="minicluster-menu-") as temporary:
    root = Path(temporary)
    (root / "sample.csv").write_text("1,1\n1,3\n8,8\n8,10\n")
    output = run("4\n4\n3\n", root)
    for name in ("results", "results-1"):
        data = json.loads((root / name / "results.json").read_text())
        assert (root / name / "clusters.svg").is_file()
        assert "centroid" in str(data)
    assert "centroid (1,2), size 2" in output
    assert "centroid (8,9), size 2" in output
    print("PASS: repeatable quick demo, centroids/sizes and separate exports")
    output = run("1\nmissing.csv\n\n0\n9\n2\nwrong\n\n-1\n\n0\n\n-1\n\nresults\ncustom\n9\n1\n1\n2\n3\n", root)
    assert (root / "custom" / "clusters.svg").is_file(), output
    assert "That path already exists" in output
    assert "Choose a different feature" in output
    assert output.index("X-axis feature") < output.index("Y-axis feature")
    print("PASS: invalid fields recover, existing output protected, axes checked")
    assert "Analysis cancelled" in run("1\n/back\n3\n", root)
    run("1\n", root)
    output = run("invalid\n5\n2\n3\n", root)
    assert "K-MEANS IN THREE STEPS" in output and "Usage:" in output
    (root / "one.csv").write_text("5\n")
    output = run("1\none.csv\n\n\n\n\n\nsingle\n3\n", root)
    assert "centroid (5), size 1" in output
    assert not (root / "single" / "clusters.svg").exists()
    print("PASS: cancellation, EOF, help and single-point/1D defaults")
