import csv
from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator


PROJECT_ROOT = Path(__file__).resolve().parents[2]

SUMMARY_PATH = (
        PROJECT_ROOT
        / "results"
        / "index-process-summary-20261116-211106.csv"
)

RAW_PATH = (
        PROJECT_ROOT
        / "results"
        / "index-process-raw-20261116-211106.csv"
)

OUTPUT_DIRECTORY = PROJECT_ROOT / "results" / "plots"

# Спільна палітра обох графіків.
GREEN = "#20D957"
LIGHT_GREEN = "#9BDCAE"
TEXT = "#203B29"
GRID = "#E2F0E6"
AXIS = "#CBD5E1"

FIGURE_SIZE = (9, 5.5)
LINE_WIDTH = 2.3
MARKER_SIZE = 7


def read_csv(path):
    if not path.is_file():
        raise FileNotFoundError(f"CSV file not found: {path}")

    with path.open(encoding="utf-8-sig", newline="") as file:
        return list(csv.DictReader(file))


def configure_style():
    plt.rcParams.update({
        "font.family": "DejaVu Sans",
        "font.size": 11,
        "text.color": TEXT,
        "axes.labelcolor": TEXT,
        "axes.titleweight": "bold",
        "axes.titlesize": 15,
        "axes.titlepad": 20,
        "axes.labelsize": 12,
        "axes.labelpad": 12,
        "axes.edgecolor": AXIS,
        "axes.spines.top": False,
        "axes.spines.right": False,
        "xtick.color": TEXT,
        "ytick.color": TEXT,
        "figure.facecolor": "white",
        "axes.facecolor": "white",
        "savefig.facecolor": "white",
        "svg.fonttype": "none",
    })


def create_chart(threads, title, ylabel, upper_limit):
    fig, ax = plt.subplots(
        figsize=FIGURE_SIZE,
        layout="constrained",
    )

    ax.set_title(title)
    ax.set_xlabel("Кількість робочих потоків")
    ax.set_ylabel(ylabel)

    ax.set_xticks(threads)
    ax.set_xlim(0.5, max(threads) + 0.8)
    ax.set_ylim(0, upper_limit)

    ax.yaxis.set_major_locator(MaxNLocator(nbins=6))

    ax.set_axisbelow(True)
    ax.grid(axis="y", color=GRID, linewidth=0.9)
    ax.tick_params(axis="both", length=0, pad=8)

    return fig, ax


def add_value_labels(ax, threads, values, anchors=None):
    # За потреби піднімаємо підписи над діапазоном вимірювань.
    if anchors is None:
        anchors = values

    for thread_count, value, anchor in zip(
            threads, values, anchors
    ):
        ax.annotate(
            f"{value:.2f}".replace(".", ","),
            xy=(thread_count, anchor),
            xytext=(0, 10),
            textcoords="offset points",
            ha="center",
            fontsize=11,
            fontweight="bold",
            color=GREEN,
            bbox={
                "facecolor": "white",
                "edgecolor": "none",
                "alpha": 0.85,
                "pad": 1.5,
            },
            zorder=5,
        )


def save_figure(fig, name):
    for extension in ("png", "svg"):
        path = OUTPUT_DIRECTORY / f"{name}.{extension}"

        fig.savefig(
            path,
            dpi=300,
            bbox_inches="tight",
        )

        print(f"Saved: {path}")

    plt.close(fig)


def plot_build_time(summary, raw):
    threads = [int(row["Threads"]) for row in summary]

    means = [
        float(row["MeanMs"]) / 1000
        for row in summary
    ]

    minimums = [
        float(row["MinMs"]) / 1000
        for row in summary
    ]

    maximums = [
        float(row["MaxMs"]) / 1000
        for row in summary
    ]

    lower_errors = [
        mean - minimum
        for mean, minimum in zip(means, minimums)
    ]

    upper_errors = [
        maximum - mean
        for mean, maximum in zip(means, maximums)
    ]

    fig, ax = create_chart(
        threads,
        title="Час побудови інвертованого індексу",
        ylabel="Час побудови, с",
        upper_limit=max(maximums) * 1.2,
    )

    ax.scatter(
        [int(row["Threads"]) for row in raw],
        [float(row["TimeMs"]) / 1000 for row in raw],
        s=35,
        color=LIGHT_GREEN,
        alpha=0.75,
        edgecolors="white",
        linewidths=0.5,
        label="Окремі вимірювання",
        zorder=3,
    )

    ax.errorbar(
        threads,
        means,
        yerr=[lower_errors, upper_errors],
        fmt="o-",
        color=GREEN,
        linewidth=LINE_WIDTH,
        markersize=MARKER_SIZE,
        elinewidth=1.5,
        capsize=5,
        label="Середнє та діапазон мінімум-максимум",
        zorder=4,
    )

    add_value_labels(
        ax,
        threads,
        means,
        anchors=maximums,
    )

    ax.legend(
        loc="upper right",
        frameon=False,
        fontsize=9,
    )

    save_figure(fig, "index-build-time")


def plot_speedup(summary):
    threads = [int(row["Threads"]) for row in summary]

    baseline = next(
        float(row["MeanMs"])
        for row in summary
        if int(row["Threads"]) == 1
    )

    speedups = [
        baseline / float(row["MeanMs"])
        for row in summary
    ]

    fig, ax = create_chart(
        threads,
        title="Прискорення побудови інвертованого індексу",
        ylabel="Прискорення S(p) = T₁ / Tₚ",
        upper_limit=max(speedups) * 1.2,
    )

    ax.plot(
        threads,
        speedups,
        "o-",
        color=GREEN,
        linewidth=LINE_WIDTH,
        markersize=MARKER_SIZE,
        zorder=4,
    )

    add_value_labels(ax, threads, speedups)

    save_figure(fig, "index-build-speedup")


def main():
    summary = read_csv(SUMMARY_PATH)
    raw = read_csv(RAW_PATH)

    if not summary or not raw:
        raise ValueError("CSV files must contain measurements.")

    summary.sort(key=lambda row: int(row["Threads"]))

    if not any(int(row["Threads"]) == 1 for row in summary):
        raise ValueError("Single-thread measurements are missing.")

    if any(float(row["MeanMs"]) <= 0 for row in summary):
        raise ValueError("Mean build time must be positive.")

    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)

    configure_style()
    plot_build_time(summary, raw)
    plot_speedup(summary)

    print("\nAll charts generated successfully.")


if __name__ == "__main__":
    main()