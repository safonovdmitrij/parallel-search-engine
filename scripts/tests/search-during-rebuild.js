(async () => {
    const query = "computer";

    async function post(path, body) {
        const controller = new AbortController();
        const timer = setTimeout(() => controller.abort(), 1200000);

        try {
            const response = await fetch(path, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(body),
                signal: controller.signal
            });

            return {
                status: response.status,
                data: await response.json()
            };
        } finally {
            clearTimeout(timer);
        }
    }

    function signature(data) {
        if (!Array.isArray(data.results)) {
            throw new Error("Відсутній масив results");
        }

        return JSON.stringify(data.results.map(item => ({
            id: item.doc_id,
            filename: item.filename,
            score: item.score,
            terms: [...item.matched_terms].sort()
        })));
    }

    const baseline = await post("/api/search", {
        query,
        mode: "keyword"
    });

    if (baseline.status !== 200 || baseline.data.total < 1) {
        throw new Error(
            "Контрольний запит має успішно знаходити документи. " +
            "За потреби зміни query на термін із датасету."
        );
    }

    const expected = signature(baseline.data);

    console.log("Запускається перебудова з одним потоком...");

    const rebuilding = post("/api/index", { threads: 1 });

    try {
        const conflictBefore = await post("/api/index", { threads: 2 });

        if (conflictBefore.status !== 409) {
            console.log(
                "Перекриття операцій не підтверджено. " +
                "Повторний запит повернув HTTP",
                conflictBefore.status
            );
            return;
        }

        const during = await post("/api/search", {
            query,
            mode: "keyword"
        });

        const conflictAfter = await post("/api/index", { threads: 2 });
        const rebuilt = await rebuilding;

        const after = await post("/api/search", {
            query,
            mode: "keyword"
        });

        console.table([
            {
                test: "Повторну перебудову відхилено",
                passed: conflictBefore.status === 409
            },
            {
                test: "Перебудова тривала до і після пошукового запиту",
                passed: conflictBefore.status === 409 &&
                    conflictAfter.status === 409
            },
            {
                test: "Пошук між перевірками зайнятості коректний",
                passed: during.status === 200 &&
                    during.data.total === baseline.data.total &&
                    signature(during.data) === expected
            },
            {
                test: "Основна перебудова успішна",
                passed: rebuilt.status === 200
            },
            {
                test: "Результати після перебудови незмінні",
                passed: after.status === 200 &&
                    after.data.total === baseline.data.total &&
                    signature(after.data) === expected
            }
        ]);
    } finally {
        await rebuilding;
    }
})().catch(error => console.error("Тест не завершено:", error));