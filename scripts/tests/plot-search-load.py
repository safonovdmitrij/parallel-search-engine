import csv
import math
import statistics
from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator


PROJECT_ROOT = Path(__file__).resolve().parents[2]

SUMMARY_PATH = (
        PROJECT_ROOT
        / "results"
        / "search-load-summary-20261117-112642.csv"
)

RAW_PATH = (
        PROJECT_ROOT
        / "results"
        / "search-load-raw-20261117-112642.csv"
)

OUTPUT_DIRECTORY = PROJECT_ROOT / "results" / "plots"

GREEN = "#20D957"
DARK_GREEN = "#128536"
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


def create_chart(x, title, ylabel, upper_limit):
    fig, ax = plt.subplots(
        figsize=FIGURE_SIZE,
        layout="constrained",
    )

    ax.set_title(title)
    ax.set_xlabel("Кількість одночасних клієнтів")
    ax.set_ylabel(ylabel)

    ax.set_xticks(x)
    ax.set_xlim(min(x) - 0.8, max(x) + 1.5)
    ax.set_ylim(0, upper_limit)

    ax.yaxis.set_major_locator(MaxNLocator(nbins=6))

    ax.set_axisbelow(True)
    ax.grid(axis="y", color=GRID, linewidth=0.9)
    ax.tick_params(axis="both", length=0, pad=8)

    return fig, ax


def add_value_labels(ax, x, values, suffix=""):
    for position, value in zip(x, values):
        label = f"{value:.2f}".replace(".", ",") + suffix

        ax.annotate(
            label,
            xy=(position, value),
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


def percentile95(values):
    ordered = sorted(values)
    index = math.ceil(0.95 * len(ordered)) - 1

    return ordered[index]


def aggregate_results(summary, raw):
    if not summary or not raw:
        raise ValueError("CSV files must contain measurements.")

    client_counts = sorted({
        int(row["Clients"])
        for row in summary
    })

    results = []

    for clients in client_counts:
        runs = [
            row for row in summary
            if int(row["Clients"]) == clients
        ]

        requests = [
            row for row in raw
            if int(row["Clients"]) == clients
        ]

        successful = [
            float(row["LatencyMs"])
            for row in requests
            if row["Passed"].strip().lower() == "true"
        ]

        if not requests or not successful:
            raise ValueError(
                f"No successful measurements for clients={clients}."
            )

        failed = len(requests) - len(successful)

        if len(requests) != sum(
                int(row["Requests"]) for row in runs
        ):
            raise ValueError(
                f"Request count mismatch for clients={clients}."
            )

        if failed != sum(int(row["Failed"]) for row in runs):
            raise ValueError(
                f"Failure count mismatch for clients={clients}."
            )

        results.append({
            "Clients": clients,
            "Runs": len(runs),
            "Requests": len(requests),
            "Successful": len(successful),
            "Failed": failed,

            # Середнє арифметичне RPS трьох проходів.
            "MeanRPS": statistics.mean(
                float(row["SuccessfulRPS"])
                for row in runs
            ),

            # Затримки за всіма успішними запитами.
            "MeanLatencyS": statistics.mean(successful) / 1000,
            "P95LatencyS": percentile95(successful) / 1000,

            # Частка помилок за всіма запитами.
            "ErrorPercent": 100 * failed / len(requests),
        })

    return results


def plot_throughput(results):
    clients = [row["Clients"] for row in results]
    rps = [row["MeanRPS"] for row in results]

    fig, ax = create_chart(
        clients,
        title="Пропускна здатність пошукової системи",
        ylabel="Успішні запити за секунду",
        upper_limit=max(rps) * 1.25,
    )

    ax.plot(
        clients,
        rps,
        "o-",
        color=GREEN,
        linewidth=LINE_WIDTH,
        markersize=MARKER_SIZE,
        zorder=4,
    )

    add_value_labels(ax, clients, rps)

    save_figure(fig, "search-load-throughput")


def plot_latency(results):
    clients = [row["Clients"] for row in results]

    means = [
        row["MeanLatencyS"]
        for row in results
    ]

    p95 = [
        row["P95LatencyS"]
        for row in results
    ]

    fig, ax = create_chart(
        clients,
        title="Затримка виконання пошукових запитів",
        ylabel="Затримка успішного запиту, с",
        upper_limit=max(p95) * 1.2,
    )

    ax.plot(
        clients,
        means,
        "o-",
        color=GREEN,
        linewidth=LINE_WIDTH,
        markersize=MARKER_SIZE,
        label="Середня затримка",
        zorder=4,
    )

    ax.plot(
        clients,
        p95,
        "s--",
        color=DARK_GREEN,
        linewidth=LINE_WIDTH,
        markersize=MARKER_SIZE,
        label="95-й процентиль (P95)",
        zorder=3,
    )

    ax.legend(
        loc="upper left",
        frameon=False,
        fontsize=10,
    )

    save_figure(fig, "search-load-latency")


def plot_errors(results):
    clients = [row["Clients"] for row in results]

    percentages = [
        row["ErrorPercent"]
        for row in results
    ]

    # Для стовпчикової діаграми конфігурації розміщені
    # рівномірно як окремі категорії.
    positions = list(range(len(clients)))

    fig, ax = create_chart(
        positions,
        title="Частка невдалих пошукових запитів",
        ylabel="Невдалі запити, %",
        upper_limit=max(5, max(percentages) * 1.3),
    )

    ax.set_xlim(-0.6, len(clients) - 0.4)
    ax.set_xticks(positions)
    ax.set_xticklabels(clients)

    ax.bar(
        positions,
        percentages,
        width=0.55,
        color=GREEN,
        zorder=3,
    )

    add_value_labels(
        ax,
        positions,
        percentages,
        suffix=" %",
    )

    save_figure(fig, "search-load-errors")


def save_aggregated_csv(results):
    path = OUTPUT_DIRECTORY / "search-load-aggregated.csv"

    with path.open(
            "w",
            encoding="utf-8-sig",
            newline="",
    ) as file:
        writer = csv.DictWriter(
            file,
            fieldnames=list(results[0].keys()),
        )

        writer.writeheader()
        writer.writerows(results)

    print(f"Saved: {path}")


def main():
    summary = read_csv(SUMMARY_PATH)
    raw = read_csv(RAW_PATH)

    results = aggregate_results(summary, raw)

    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)

    configure_style()

    plot_throughput(results)
    plot_latency(results)
    plot_errors(results)

    save_aggregated_csv(results)

    print("\nAll load-test charts generated successfully.")


if __name__ == "__main__":
    main()