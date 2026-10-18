#!/usr/bin/env python3
import argparse
import json
import math
import sys

ENVIRONMENT_KEYS = (
    "cpu_model",
    "logical_cores",
    "physical_cores",
    "compiler",
    "compiler_version",
    "build_type",
    "os_name",
    "kernel_version",
)

TIMING_METRICS = (
    ("median_ns", "timing.stats.median"),
    ("mean_ns", "timing.stats.mean"),
    ("stddev_ns", "timing.stats.stddev"),
    ("min_ns", "timing.stats.minimum"),
    ("max_ns", "timing.stats.maximum"),
)

MEMORY_METRICS = (
    ("peak_rss_bytes", "memory.peak_rss_bytes"),
    ("peak_live_bytes", "memory.peak_live_bytes"),
    ("allocation_count", "memory.allocation_count"),
)


def load_report(path):
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def dig(document, dotted):
    current = document
    for key in dotted.split("."):
        if not isinstance(current, dict) or key not in current:
            return None
        current = current[key]
    return current


def environment_signature(document):
    environment = document.get("environment", {})
    return tuple(environment.get(key) for key in ENVIRONMENT_KEYS)


def describe_environment(document):
    environment = document.get("environment", {})
    return (
        f"{environment.get('cpu_model', 'unknown')} | cores={environment.get('logical_cores', '?')} | "
        f"{environment.get('compiler', '?')} {environment.get('compiler_version', '?')} | "
        f"{environment.get('build_type', '?')} | {environment.get('os_name', '?')} "
        f"{environment.get('kernel_version', '?')}"
    )


def distributions_overlap(baseline_stats, candidate_stats):
    if not baseline_stats or not candidate_stats:
        return True
    baseline_low = baseline_stats["mean"] - baseline_stats["stddev"]
    baseline_high = baseline_stats["mean"] + baseline_stats["stddev"]
    candidate_low = candidate_stats["mean"] - candidate_stats["stddev"]
    candidate_high = candidate_stats["mean"] + candidate_stats["stddev"]
    return baseline_low <= candidate_high and candidate_low <= baseline_high


def verdict(baseline, candidate, overlap):
    if overlap:
        return "within_noise"
    if baseline == 0.0:
        return "improved" if candidate < baseline else "regressed"
    return "improved" if candidate < baseline else "regressed"


def relative_delta(baseline, candidate):
    if baseline in (None, 0.0) or candidate is None:
        return None
    return (candidate - baseline) / baseline


def format_value(value):
    if value is None:
        return "n/a"
    if isinstance(value, float):
        if math.isnan(value) or math.isinf(value):
            return "n/a"
        return f"{value:.6g}"
    return str(value)


def compare_pair(baseline, candidate):
    rows = []
    baseline_stats = dig(baseline, "timing.stats")
    candidate_stats = dig(candidate, "timing.stats")
    overlap = distributions_overlap(baseline_stats, candidate_stats)

    for name, path in TIMING_METRICS:
        baseline_value = dig(baseline, path)
        candidate_value = dig(candidate, path)
        if baseline_value is None and candidate_value is None:
            continue
        delta = relative_delta(baseline_value, candidate_value)
        rows.append((name, baseline_value, candidate_value, delta, verdict(baseline_value, candidate_value, overlap)))

    for name, path in MEMORY_METRICS:
        baseline_value = dig(baseline, path)
        candidate_value = dig(candidate, path)
        if baseline_value is None and candidate_value is None:
            continue
        delta = relative_delta(float(baseline_value), float(candidate_value))
        rows.append((name, baseline_value, candidate_value, delta, verdict(float(baseline_value),
                                                                          float(candidate_value), overlap)))
    return rows


def main():
    parser = argparse.ArgumentParser(description="Noise-aware comparison of two or more AetherQ JSON reports")
    parser.add_argument("reports", nargs="+", help="JSON report files; the first is the baseline")
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    arguments = parser.parse_args()

    if len(arguments.reports) < 2:
        print("at least two reports are required", file=sys.stderr)
        return 2

    documents = [load_report(path) for path in arguments.reports]
    baseline = documents[0]
    baseline_signature = environment_signature(baseline)
    baseline_description = describe_environment(baseline)

    results = []
    mismatch = False
    for path, candidate in zip(arguments.reports[1:], documents[1:]):
        if environment_signature(candidate) != baseline_signature:
            mismatch = True
            print(f"WARNING: environment differs between {arguments.reports[0]} and {path}; "
                  f"comparison is not a controlled experiment", file=sys.stderr)
            print(f"  baseline:  {baseline_description}", file=sys.stderr)
            print(f"  candidate: {describe_environment(candidate)}", file=sys.stderr)
        results.append({"path": path, "rows": compare_pair(baseline, candidate)})

    if arguments.json:
        payload = {
            "baseline": arguments.reports[0],
            "environment_mismatch": mismatch,
            "comparisons": [
                {
                    "candidate": entry["path"],
                    "metrics": [
                        {
                            "name": row[0],
                            "baseline": row[1],
                            "candidate": row[2],
                            "relative_delta": row[3],
                            "verdict": row[4],
                        }
                        for row in entry["rows"]
                    ],
                }
                for entry in results
            ],
        }
        print(json.dumps(payload, indent=2))
        return 1 if mismatch else 0

    print(f"baseline: {arguments.reports[0]}")
    print(f"  environment: {baseline_description}")
    for entry in results:
        print(f"candidate: {entry['path']}")
        print("  metric,baseline,candidate,relative_delta,verdict")
        for row in entry["rows"]:
            print(f"  {row[0]},{format_value(row[1])},{format_value(row[2])},"
                  f"{format_value(row[3])},{row[4]}")
    return 1 if mismatch else 0


if __name__ == "__main__":
    sys.exit(main())
