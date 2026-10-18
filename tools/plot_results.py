#!/usr/bin/env python3
import argparse
import json
import os
import sys
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def load_report(path):
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def is_run_report(document):
    return "timing" in document and "workload" in document


def is_scaling_report(document):
    return "rows" in document and isinstance(document["rows"], list)


def run_label(document):
    workload = document.get("workload", "unknown")
    threads = document.get("execution", {}).get("threads", document.get("config", {}).get("threads", 0))
    backend = document.get("backend", "unknown")
    return f"{workload}/{backend}/{threads}t"


def median_seconds(document):
    samples = document.get("timing", {}).get("samples_ns", [])
    if not samples:
        return float("nan")
    ordered = sorted(samples)
    middle = len(ordered) // 2
    if len(ordered) % 2 == 1:
        return ordered[middle] / 1e9
    return (ordered[middle - 1] + ordered[middle]) / 2.0 / 1e9


def throughput(document):
    for metric in document.get("metrics", []):
        if metric.get("name") in {"paths_per_second", "options_per_second", "items_per_second"}:
            return metric.get("value", float("nan"))
    return float("nan")


def percentile(samples, quantile):
    if not samples:
        return float("nan")
    ordered = sorted(samples)
    if len(ordered) == 1:
        return ordered[0] / 1e6
    position = quantile * (len(ordered) - 1)
    lower = int(position)
    upper = min(lower + 1, len(ordered) - 1)
    fraction = position - lower
    value = ordered[lower] + (ordered[upper] - ordered[lower]) * fraction
    return value / 1e6


def plot_scaling(runs, output_dir):
    grouped = defaultdict(list)
    for document in runs:
        workload = document.get("workload", "unknown")
        threads = document.get("execution", {}).get("threads", 1)
        grouped[(workload, document.get("backend", "unknown"))].append((threads, median_seconds(document)))

    created = []
    fig, axes = plt.subplots(1, 2, figsize=(11, 4.5))
    for (workload, backend), points in sorted(grouped.items()):
        points.sort()
        threads = [point[0] for point in points]
        seconds = [point[1] for point in points]
        if not threads:
            continue
        baseline = seconds[0]
        axes[0].plot(threads, seconds, marker="o", label=f"{workload} ({backend})")
        speedup = [baseline / value if value else float("nan") for value in seconds]
        axes[1].plot(threads, speedup, marker="o", label=f"{workload} ({backend})")
        axes[1].plot(threads, threads, linestyle="--", color="grey", linewidth=1)

    axes[0].set_xlabel("threads")
    axes[0].set_ylabel("median wall time (s)")
    axes[0].set_title("wall time vs threads")
    axes[0].grid(True, alpha=0.3)
    axes[0].legend(fontsize=8)

    axes[1].set_xlabel("threads")
    axes[1].set_ylabel("speedup vs 1 thread")
    axes[1].set_title("speedup vs threads")
    axes[1].grid(True, alpha=0.3)
    axes[1].legend(fontsize=8)

    fig.tight_layout()
    path = os.path.join(output_dir, "scaling.png")
    fig.savefig(path, dpi=120)
    plt.close(fig)
    created.append(path)
    return created


def plot_throughput(runs, output_dir):
    grouped = defaultdict(list)
    for document in runs:
        workload = document.get("workload", "unknown")
        threads = document.get("execution", {}).get("threads", 1)
        value = throughput(document)
        if value == value:
            grouped[(workload, document.get("backend", "unknown"))].append((threads, value))

    if not grouped:
        return []

    fig, axis = plt.subplots(figsize=(7, 4.5))
    for (workload, backend), points in sorted(grouped.items()):
        points.sort()
        axis.plot([point[0] for point in points], [point[1] for point in points], marker="o",
                  label=f"{workload} ({backend})")
    axis.set_xlabel("threads")
    axis.set_ylabel("throughput (items/s)")
    axis.set_title("throughput vs threads")
    axis.grid(True, alpha=0.3)
    axis.legend(fontsize=8)
    fig.tight_layout()
    path = os.path.join(output_dir, "throughput.png")
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return [path]


def plot_latency(runs, output_dir):
    quantiles = [0.5, 0.95, 0.99]
    labels = []
    series = []
    for document in runs:
        samples = document.get("timing", {}).get("samples_ns", [])
        if not samples:
            continue
        labels.append(run_label(document))
        series.append([percentile(samples, quantile) for quantile in quantiles])

    if not series:
        return []

    width = 0.8 / len(series)
    positions = range(len(quantiles))
    fig, axis = plt.subplots(figsize=(7, 4.5))
    for index, values in enumerate(series):
        offset = [position + index * width for position in positions]
        axis.bar(offset, values, width=width, label=labels[index])
    axis.set_xticks([position + 0.4 - width / 2 for position in positions])
    axis.set_xticklabels([f"p{int(quantile * 100)}" for quantile in quantiles])
    axis.set_ylabel("latency (ms)")
    axis.set_title("latency percentiles")
    axis.grid(True, axis="y", alpha=0.3)
    axis.legend(fontsize=8)
    fig.tight_layout()
    path = os.path.join(output_dir, "latency.png")
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return [path]


def plot_memory(runs, output_dir):
    grouped = defaultdict(list)
    for document in runs:
        workload = document.get("workload", "unknown")
        parameters = document.get("workload_parameters", {})
        size = None
        for key in ("paths", "options", "items", "observations"):
            if key in parameters:
                size = parameters[key]
                break
        peak_rss = document.get("memory", {}).get("peak_rss_bytes", 0)
        if size is not None:
            grouped[workload].append((size, peak_rss / (1024.0 * 1024.0)))

    if not grouped:
        return []

    fig, axis = plt.subplots(figsize=(7, 4.5))
    for workload, points in sorted(grouped.items()):
        points.sort()
        axis.plot([point[0] for point in points], [point[1] for point in points], marker="o", label=workload)
    axis.set_xlabel("problem size")
    axis.set_ylabel("peak RSS (MiB)")
    axis.set_title("peak memory vs problem size")
    axis.grid(True, alpha=0.3)
    axis.legend(fontsize=8)
    fig.tight_layout()
    path = os.path.join(output_dir, "memory.png")
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return [path]


def plot_backend_compare(runs, output_dir):
    grouped = defaultdict(lambda: defaultdict(list))
    for document in runs:
        workload = document.get("workload", "unknown")
        backend = document.get("backend", "unknown")
        threads = document.get("execution", {}).get("threads", 1)
        grouped[workload][backend].append((threads, median_seconds(document)))

    if not grouped:
        return []

    workloads = sorted(grouped)
    fig, axis = plt.subplots(figsize=(7, 4.5))
    width = 0.8 / max(1, len({backend for entry in grouped.values() for backend in entry}))
    backends = sorted({backend for entry in grouped.values() for backend in entry})
    for index, backend in enumerate(backends):
        heights = []
        for workload in workloads:
            points = sorted(grouped[workload].get(backend, []))
            heights.append(points[0][1] if points else float("nan"))
        positions = [position + index * width for position in range(len(workloads))]
        axis.bar(positions, heights, width=width, label=backend)
    axis.set_xticks([position + 0.4 - width / 2 for position in range(len(workloads))])
    axis.set_xticklabels(workloads, rotation=15, fontsize=8)
    axis.set_ylabel("median wall time (s)")
    axis.set_title("scalar vs optimized backend")
    axis.grid(True, axis="y", alpha=0.3)
    axis.legend(fontsize=8)
    fig.tight_layout()
    path = os.path.join(output_dir, "backend_compare.png")
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return [path]


def main():
    parser = argparse.ArgumentParser(description="Render PNG plots from AetherQ JSON reports")
    parser.add_argument("reports", nargs="+", help="JSON report files produced by the aetherq CLI")
    parser.add_argument("--out", default="results", help="output directory for PNG files")
    arguments = parser.parse_args()

    documents = [load_report(path) for path in arguments.reports]
    runs = [document for document in documents if is_run_report(document)]
    scaling_rows = []
    for document in documents:
        if is_scaling_report(document):
            for row in document["rows"]:
                scaling_rows.append(row)

    if not runs and not scaling_rows:
        print("no run or scaling reports found", file=sys.stderr)
        return 1

    os.makedirs(arguments.out, exist_ok=True)
    created = []
    created.extend(plot_scaling(runs, arguments.out))
    created.extend(plot_throughput(runs, arguments.out))
    created.extend(plot_latency(runs, arguments.out))
    created.extend(plot_memory(runs, arguments.out))
    created.extend(plot_backend_compare(runs, arguments.out))

    for path in created:
        print(path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
