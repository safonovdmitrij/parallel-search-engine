import csv
import http.client
import json
import math
import random
import statistics
import threading
import time

from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
from pathlib import Path
from urllib.parse import urlsplit


BASE_URL = "http://127.0.0.1:8080"

CLIENT_COUNTS = [1, 2, 4, 8, 16, 32]
REPETITIONS = 3
DURATION_SECONDS = 20
TIMEOUT_SECONDS = 30
EXPECTED_DOCUMENTS = 2770

QUERIES = [
    "computer",
    "parallel",
    "search",
    "data processing",
    "thread pool",
    "xqzvnonexistenttoken",
]

PROJECT_ROOT = Path(__file__).resolve().parents[2]
OUTPUT_DIRECTORY = PROJECT_ROOT / "results"

ADDRESS = urlsplit(BASE_URL)

PAYLOADS = {
    query: json.dumps({
        "query": query,
        "mode": "keyword",
    }).encode("utf-8")
    for query in QUERIES
}


def new_connection():
    return http.client.HTTPConnection(
        ADDRESS.hostname,
        ADDRESS.port or 80,
        timeout=TIMEOUT_SECONDS,
        )


def get_json(path):
    connection = new_connection()

    try:
        connection.request("GET", path)
        response = connection.getresponse()
        body = response.read()

        if response.status != 200:
            raise RuntimeError(
                f"GET {path}: HTTP {response.status}"
            )

        return json.loads(body)
    finally:
        connection.close()


def send_search(connection, query):
    connection.request(
        "POST",
        "/api/search",
        body=PAYLOADS[query],
        headers={
            "Content-Type": "application/json",
            "Accept": "application/json",
        },
    )

    response = connection.getresponse()
    body = response.read()

    return response.status, body


def result_signature(data, query):
    if data.get("query") != query:
        raise ValueError("Response query mismatch.")

    if data.get("mode") != "keyword":
        raise ValueError("Response mode mismatch.")

    results = data.get("results")

    if not isinstance(results, list):
        raise ValueError("Results must be a list.")

    if data.get("total") != len(results):
        raise ValueError("Result count mismatch.")

    # Порівнюємо склад, порядок, оцінки та знайдені терміни.
    return tuple(
        (
            str(item["doc_id"]),
            item["filename"],
            item["score"],
            tuple(sorted(item["matched_terms"])),
        )
        for item in results
    )


def prepare_reference():
    stats = get_json("/api/stats")

    if stats.get("documents") != EXPECTED_DOCUMENTS:
        raise RuntimeError(
            "Unexpected document count: "
            f"{stats.get('documents')}. "
            f"Expected {EXPECTED_DOCUMENTS}."
        )

    reference = {}
    connection = new_connection()

    try:
        print("Preparing reference responses...")

        for query in QUERIES:
            status, body = send_search(connection, query)

            if status != 200:
                raise RuntimeError(
                    f"Reference query {query!r}: HTTP {status}"
                )

            data = json.loads(body)
            reference[query] = result_signature(data, query)

            print(
                f"  {query!r}: "
                f"{len(reference[query])} documents"
            )

        if not any(reference.values()):
            raise RuntimeError(
                "All reference queries returned no results. "
                "Check the dataset and query list."
            )

        return reference
    finally:
        connection.close()


def percentile95(values):
    ordered = sorted(values)
    index = math.ceil(0.95 * len(ordered)) - 1

    return ordered[index]


def run_case(clients, run, reference):
    # Усі клієнти починають вимірювання після прогрівання.
    barrier = threading.Barrier(clients + 1)
    timing = {}

    def worker(client_id):
        connection = new_connection()
        rows = []

        try:
            # Одне прогрівальне звернення для кожного клієнта.
            warmup_query = QUERIES[client_id % len(QUERIES)]
            status, body = send_search(connection, warmup_query)

            if status != 200:
                raise RuntimeError(
                    f"Warm-up failed: HTTP {status}"
                )

            signature = result_signature(
                json.loads(body),
                warmup_query,
            )

            if signature != reference[warmup_query]:
                raise RuntimeError(
                    "Warm-up result mismatch."
                )

            connection.close()
            barrier.wait(timeout=60)

            sequence = 0

            while time.perf_counter() < timing["deadline"]:
                query = QUERIES[
                    (client_id + sequence) % len(QUERIES)
                    ]

                status = 0
                passed = False
                error = ""

                started = time.perf_counter()

                try:
                    status, body = send_search(
                        connection,
                        query,
                    )

                    if status != 200:
                        raise RuntimeError(
                            f"HTTP {status}"
                        )

                    signature = result_signature(
                        json.loads(body),
                        query,
                    )

                    if signature != reference[query]:
                        raise ValueError(
                            "Search result mismatch."
                        )

                    passed = True

                except Exception as exception:
                    error = (
                                f"{type(exception).__name__}: "
                                f"{exception}"
                            )[:200]

                    # Наступний запит відкриє нове з’єднання.
                    connection.close()

                elapsed_ms = (
                                     time.perf_counter() - started
                             ) * 1000

                rows.append({
                    "Run": run,
                    "Clients": clients,
                    "Client": client_id + 1,
                    "Query": query,
                    "HttpStatus": status,
                    "Passed": passed,
                    "LatencyMs": round(elapsed_ms, 4),
                    "Error": error,
                })

                sequence += 1

            return rows, time.perf_counter()

        except Exception:
            barrier.abort()
            raise
        finally:
            connection.close()

    with ThreadPoolExecutor(max_workers=clients) as executor:
        futures = [
            executor.submit(worker, client_id)
            for client_id in range(clients)
        ]

        try:
            # Чекаємо, поки клієнти завершать прогрівання.
            timeout = time.perf_counter() + 60

            while barrier.n_waiting < clients:
                if barrier.broken:
                    raise RuntimeError(
                        "Client warm-up failed."
                    )

                if time.perf_counter() > timeout:
                    raise TimeoutError(
                        "Client warm-up timed out."
                    )

                time.sleep(0.01)

            timing["start"] = time.perf_counter()
            timing["deadline"] = (
                    timing["start"] + DURATION_SECONDS
            )

            barrier.wait(timeout=60)

        except Exception:
            barrier.abort()
            raise

        outputs = [future.result() for future in futures]

    rows = [
        row
        for client_rows, _ in outputs
        for row in client_rows
    ]

    finished = max(
        finish_time
        for _, finish_time in outputs
    )

    # Враховуємо завершення вже розпочатих запитів.
    elapsed_seconds = finished - timing["start"]

    successful = [
        row["LatencyMs"]
        for row in rows
        if row["Passed"]
    ]

    failed = len(rows) - len(successful)

    summary = {
        "Run": run,
        "Clients": clients,
        "DurationS": round(elapsed_seconds, 4),
        "Requests": len(rows),
        "Successful": len(successful),
        "Failed": failed,
        "ErrorPercent": round(
            100 * failed / len(rows), 4
        ) if rows else 0,
        "SuccessfulRPS": round(
            len(successful) / elapsed_seconds, 4
        ),
        "MeanLatencyMs": round(
            statistics.mean(successful), 4
        ) if successful else "",
        "P95LatencyMs": round(
            percentile95(successful), 4
        ) if successful else "",
    }

    return summary, rows


def main():
    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)

    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")

    raw_path = (
            OUTPUT_DIRECTORY / f"search-load-raw-{stamp}.csv"
    )

    summary_path = (
            OUTPUT_DIRECTORY / f"search-load-summary-{stamp}.csv"
    )

    reference = prepare_reference()

    summary_fields = [
        "Run", "Clients", "DurationS", "Requests",
        "Successful", "Failed", "ErrorPercent",
        "SuccessfulRPS", "MeanLatencyMs", "P95LatencyMs",
    ]

    raw_fields = [
        "Run", "Clients", "Client", "Query",
        "HttpStatus", "Passed", "LatencyMs", "Error",
    ]

    with (
        summary_path.open(
            "w", encoding="utf-8-sig", newline=""
        ) as summary_file,
        raw_path.open(
            "w", encoding="utf-8-sig", newline=""
        ) as raw_file,
    ):
        summary_writer = csv.DictWriter(
            summary_file,
            fieldnames=summary_fields,
        )

        raw_writer = csv.DictWriter(
            raw_file,
            fieldnames=raw_fields,
        )

        summary_writer.writeheader()
        raw_writer.writeheader()

        for run in range(1, REPETITIONS + 1):
            order = CLIENT_COUNTS.copy()
            random.shuffle(order)

            for clients in order:
                print(
                    f"\nRun {run}/{REPETITIONS}; "
                    f"clients={clients}"
                )

                summary, rows = run_case(
                    clients,
                    run,
                    reference,
                )

                summary_writer.writerow(summary)
                raw_writer.writerows(rows)

                summary_file.flush()
                raw_file.flush()

                print(
                    f"RPS={summary['SuccessfulRPS']:.2f}; "
                    f"mean={summary['MeanLatencyMs']} ms; "
                    f"p95={summary['P95LatencyMs']} ms; "
                    f"failed={summary['Failed']}"
                )

                time.sleep(2)

    print(f"\nRaw measurements: {raw_path}")
    print(f"Summary: {summary_path}")


if __name__ == "__main__":
    main()