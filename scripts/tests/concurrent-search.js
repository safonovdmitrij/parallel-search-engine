(async () => {
    const cases = [
        { query: "cat", expected: "01.txt:1,05.txt:1" },
        { query: "cat dog", expected: "01.txt:2,02.txt:1,05.txt:1" },
        { query: "bird", expected: "02.txt:1,03.txt:1" },
        { query: "computer", expected: "04.txt:1" },
        { query: "123", expected: "05.txt:1" },
        { query: "elephant", expected: "" }
    ];

    const clientCount = 4;
    const requestsPerClient = 20;
    const failures = [];

    async function runClient(clientId) {
        let passed = 0;

        for (let i = 0; i < requestsPerClient; i++) {
            const test = cases[(i + clientId) % cases.length];
            const controller = new AbortController();
            const timer = setTimeout(() => controller.abort(), 10000);

            try {
                const response = await fetch("/api/search", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({
                        query: test.query,
                        mode: "keyword"
                    }),
                    signal: controller.signal
                });

                if (!response.ok) {
                    throw new Error(`HTTP ${response.status}`);
                }

                const data = await response.json();

                if (!Array.isArray(data.results)) {
                    throw new Error("Поле results не є масивом");
                }

                const actual = data.results
                    .map(item => `${item.filename}:${item.score}`)
                    .join(",");

                if (
                    actual !== test.expected ||
                    data.total !== data.results.length ||
                    data.query !== test.query
                ) {
                    throw new Error(
                        `Очікується '${test.expected}', отримано '${actual}'`
                    );
                }

                passed++;
            } catch (error) {
                failures.push({
                    client: clientId + 1,
                    request: i + 1,
                    query: test.query,
                    error: error.message
                });
            } finally {
                clearTimeout(timer);
            }
        }

        return {
            client: clientId + 1,
            requests: requestsPerClient,
            passed,
            failed: requestsPerClient - passed
        };
    }

    const results = await Promise.all(
        Array.from({ length: clientCount }, (_, id) => runClient(id))
    );

    console.table(results);

    const totalPassed = results.reduce((sum, row) => sum + row.passed, 0);

    console.log(
        `Успішно: ${totalPassed}/${clientCount * requestsPerClient}`
    );

    if (failures.length > 0) {
        console.table(failures);
    }
})().catch(error => console.error("Тест не завершено:", error));