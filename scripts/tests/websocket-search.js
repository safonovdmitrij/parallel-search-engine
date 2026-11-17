(async () => {
    const query = "computer";
    const messages = [];

    const received = await new Promise((resolve, reject) => {
        const ws = new WebSocket("ws://127.0.0.1:8080/ws/search");
        let finished = false;

        const finish = (error) => {
            if (finished) return;
            finished = true;

            clearTimeout(timer);
            ws.close(1000, "Test completed");

            if (error) reject(error);
            else resolve(messages);
        };

        const timer = setTimeout(() => {
            finish(new Error("Не отримано done за 150 секунд"));
        }, 150000);

        ws.onopen = () => {
            ws.send(JSON.stringify({ query }));
        };

        ws.onmessage = (event) => {
            try {
                const message = JSON.parse(event.data);
                messages.push(message);
                console.log("Отримано:", message);

                if (message.stage === "error") {
                    finish(new Error(JSON.stringify(message.data)));
                } else if (message.stage === "done") {
                    finish();
                }
            } catch (error) {
                finish(error);
            }
        };

        ws.onerror = () => {
            finish(new Error("Помилка WebSocket-з’єднання"));
        };

        ws.onclose = (event) => {
            console.log("WebSocket закрито:", {
                code: event.code,
                reason: event.reason,
                wasClean: event.wasClean
            });

            if (!finished) {
                finish(new Error("З’єднання закрито до отримання done"));
            }
        };
    });

    const stages = received.map(message => message.stage);

    const expansion = received.find(
        message => message.stage === "query_expansion"
    )?.data;

    const search = received.find(
        message => message.stage === "keyword_search"
    )?.data;

    const document = search?.results?.find(
        result => result.filename === "04.txt"
    );

    console.table([
        {
            test: "Послідовність трьох етапів",
            passed: stages.join(",") ===
                "query_expansion,keyword_search,done"
        },
        {
            test: "Початковий запит збережено",
            passed: expansion?.original === query
        },
        {
            test: "Отримано додаткові терміни",
            passed: Array.isArray(expansion?.expanded_terms) &&
                expansion.expanded_terms.length > 0
        },
        {
            test: "Знайдено 04.txt",
            passed: Boolean(document)
        },
        {
            test: "Знайдено початковий термін computer",
            passed: document?.matched_terms?.includes(query) === true
        },
        {
            test: "Отримано коректний час пошуку",
            passed: typeof search?.time_ms === "number" &&
                search.time_ms >= 0
        }
    ]);

    console.log("Додаткові терміни:", expansion?.expanded_terms);
    console.table(search?.results ?? []);
})().catch(error => console.error("Тест не завершено:", error));