(async () => {
    const ws = new WebSocket("ws://127.0.0.1:8080/ws/search");
    const checks = [];

    await new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
            ws.close();
            reject(new Error("Не вдалося відкрити з’єднання за 5 секунд"));
        }, 5000);

        ws.onopen = () => {
            clearTimeout(timer);
            resolve();
        };

        ws.onerror = () => {
            clearTimeout(timer);
            reject(new Error("Помилка встановлення з’єднання"));
        };
    });

    function runRound(query, extraQuery = null) {
        return new Promise((resolve, reject) => {
            const messages = [];

            const cleanup = () => {
                clearTimeout(timer);
                ws.removeEventListener("message", onMessage);
                ws.removeEventListener("close", onClose);
            };

            const fail = (error) => {
                cleanup();
                reject(error);
            };

            const onClose = () => {
                fail(new Error("З’єднання закрито під час перевірки"));
            };

            const onMessage = (event) => {
                try {
                    const message = JSON.parse(event.data);
                    messages.push(message);
                    console.log(query, message);

                    if (message.stage === "error" && extraQuery === null) {
                        fail(new Error(JSON.stringify(message.data)));
                        return;
                    }

                    const hasDone = messages.some(m => m.stage === "done");
                    const hasError = messages.some(m => m.stage === "error");

                    if (hasDone && (extraQuery === null || hasError)) {
                        cleanup();
                        resolve(messages);
                    }
                } catch (error) {
                    fail(error);
                }
            };

            const timer = setTimeout(() => {
                fail(new Error(
                    "Не отримано всі очікувані повідомлення за 150 секунд"
                ));
            }, 150000);

            ws.addEventListener("message", onMessage);
            ws.addEventListener("close", onClose);

            try {
                ws.send(JSON.stringify({ query }));

                if (extraQuery !== null) {
                    ws.send(JSON.stringify({ query: extraQuery }));
                }
            } catch (error) {
                fail(error);
            }
        });
    }

    function checkRound(name, messages, query, expectedFiles) {
        const stages = messages
            .filter(m => m.stage !== "error")
            .map(m => m.stage)
            .join(",");

        const expansion = messages.find(
            m => m.stage === "query_expansion"
        )?.data;

        const results = messages.find(
            m => m.stage === "keyword_search"
        )?.data?.results ?? [];

        checks.push({
            test: name,
            passed:
                stages === "query_expansion,keyword_search,done" &&
                expansion?.original === query &&
                expectedFiles.every(filename =>
                    results.some(result => result.filename === filename)
                )
        });
    }

    try {
        const first = await runRound("computer");
        checkRound("Перший пошук", first, "computer", ["04.txt"]);

        const second = await runRound("cat");
        checkRound(
            "Другий пошук через те саме з’єднання",
            second,
            "cat",
            ["01.txt", "05.txt"]
        );

        const busy = await runRound("computer", "bird");
        checkRound(
            "Перший запит завершується попри відхилення другого",
            busy,
            "computer",
            ["04.txt"]
        );

        const errors = busy.filter(m => m.stage === "error");

        checks.push({
            test: "Другий одночасний запит відхилено через зайнятість",
            passed:
                errors.length === 1 &&
                typeof errors[0].data?.error === "string" &&
                /already|running|busy|progress/i.test(errors[0].data.error)
        });

        checks.push({
            test: "З’єднання залишається відкритим",
            passed: ws.readyState === WebSocket.OPEN
        });

        console.table(checks);
        console.log("Повідомлення про зайнятість:", errors[0]?.data);
    } finally {
        ws.onclose = event => {
            console.log("Закриття:", {
                code: event.code,
                wasClean: event.wasClean
            });
        };

        ws.close(1000, "Session tests completed");
    }
})().catch(error => console.error("Тест не завершено:", error));