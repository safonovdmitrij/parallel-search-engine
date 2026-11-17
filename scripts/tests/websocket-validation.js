(async () => {
    const ws = new WebSocket("ws://127.0.0.1:8080/ws/search");

    await new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
            ws.close();
            reject(new Error("Час очікування з’єднання вичерпано"));
        }, 5000);

        ws.onopen = () => {
            clearTimeout(timer);
            resolve();
        };

        ws.onerror = () => {
            clearTimeout(timer);
            reject(new Error("Не вдалося встановити з’єднання"));
        };
    });

    function sendAndReceive(payload) {
        return new Promise((resolve, reject) => {
            const cleanup = () => {
                clearTimeout(timer);
                ws.removeEventListener("message", onMessage);
                ws.removeEventListener("close", onClose);
            };

            const onMessage = (event) => {
                cleanup();

                try {
                    resolve(JSON.parse(event.data));
                } catch (error) {
                    reject(error);
                }
            };

            const onClose = () => {
                cleanup();
                reject(new Error("З’єднання закрито до відповіді"));
            };

            const timer = setTimeout(() => {
                cleanup();
                reject(new Error("Немає відповіді протягом 5 секунд"));
            }, 5000);

            ws.addEventListener("message", onMessage);
            ws.addEventListener("close", onClose);

            try {
                ws.send(payload);
            } catch (error) {
                cleanup();
                reject(error);
            }
        });
    }

    const cases = [
        { name: "Некоректний JSON", payload: '{"query":' },
        { name: "Відсутнє поле query", payload: '{}' },
        { name: "Порожній запит", payload: '{"query":""}' },
        { name: "Лише пробіли", payload: '{"query":"   "}' },
        { name: "Числовий query", payload: '{"query":42}' },
        { name: "query дорівнює null", payload: '{"query":null}' },
        {
            name: "Запит довжиною 1001 байт",
            payload: JSON.stringify({ query: "z".repeat(1001) })
        },
        {
            name: "Бінарне повідомлення",
            payload: new Blob(['{"query":"computer"}'])
        }
    ];

    const results = [];

    try {
        for (const test of cases) {
            const response = await sendAndReceive(test.payload);

            results.push({
                test: test.name,
                expected: "error із непорожнім data.error",
                actual: response.stage,
                passed: response.stage === "error" &&
                    typeof response.data?.error === "string" &&
                    response.data.error.trim().length > 0,
                message: response.data?.error ?? ""
            });
        }

        console.table(results);

        console.log(
            "З’єднання залишається відкритим:",
            ws.readyState === WebSocket.OPEN
        );
    } finally {
        ws.onclose = (event) => {
            console.log("Закриття:", {
                code: event.code,
                wasClean: event.wasClean
            });
        };

        ws.close(1000, "Validation completed");
    }
})().catch(error => console.error("Тест не завершено:", error));