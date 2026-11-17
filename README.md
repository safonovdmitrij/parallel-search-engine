# Parallel Search Engine

Пошукова система на C++20 із паралельною побудовою інвертованого індексу, власним пулом потоків та розширенням запитів через Ollama. Доступні вебінтерфейс, CLI, REST API та WebSocket.

## Перед запуском

- Встановіть і запустіть Docker із Docker Compose v2. Для Windows - Docker Desktop із WSL 2.
- Звільніть порт `8080`.
- Під час першого запуску потрібен інтернет для збірки та завантаження моделі.

## 1. Завантажте проєкт

```sh
git clone https://github.com/safonovdmitrij/parallel-search-engine.git
cd parallel-search-engine
```

## 2. Запустіть систему

За замовчуванням використовується демонстраційний набір із **п’яти документів** у `data/functional/`, який входить до репозиторію. Для першого запуску додатково завантажувати дані або створювати `.env` не потрібно.

```sh
docker compose up -d
docker compose logs -f server
```

Compose збере сервер, запустить Ollama та завантажить модель `gemma3:1b`. Дочекайтеся повідомлення `Index built.` і запуску HTTP-сервера. Перший запуск може тривати кілька хвилин. Натисніть `Ctrl+C`, щоб вийти з перегляду логів; система продовжить працювати.

## 3. Відкрийте вебінтерфейс

Перейдіть на [http://127.0.0.1:8080](http://127.0.0.1:8080).

- **keyword** - пошук за ключовими словами.
- **llm** - розширення запиту через Ollama з відображенням етапів пошуку.
- **Керування індексом** - перебудова із заданою кількістю робочих потоків.

Перевірка готовності в PowerShell:

```powershell
Invoke-RestMethod http://127.0.0.1:8080/api/health
Invoke-RestMethod http://127.0.0.1:8080/api/stats
```

Для демонстраційного набору очікуйте `status: ok`, `ollama: true` та `documents: 5`. Спробуйте пошук `cat dog` або `computer`. Якщо `ollama: false`, звичайний пошук залишається доступним.

## Власний датасет

1. Підготуйте каталог англомовних текстових файлів із розширенням `.txt`. Кожен файл є окремим документом; підкаталоги обробляються автоматично. Архів потрібно розпакувати. Рекомендоване кодування - UTF-8 без BOM.
2. Створіть файл `.env` у корені проєкту та вкажіть шлях до каталогу:

```dotenv
DATASET_DIR=./data/raw/D1.7GB
```

Можна використати абсолютний шлях, наприклад у Windows:

```dotenv
DATASET_DIR=D:/datasets/documents
```

3. Застосуйте зміну й дочекайтеся побудови нового індексу:

```sh
docker compose up -d --force-recreate server
docker compose logs -f server
```

Після запуску перевірте кількість документів через `/api/stats`. Власний датасет зберігається на вашому комп’ютері та підключається до контейнера лише для читання.

Щоб повернути демонстраційний набір, видаліть рядок `DATASET_DIR` із `.env` і повторіть команду перестворення сервера.

## CLI

Після запуску сервера виконайте:

```sh
docker compose exec server /app/build/search "computer" keyword
docker compose exec server /app/build/search "computer" llm
```

Синтаксис окремо зібраного CLI: `search "query" [keyword|llm] [server_url]`. За замовчуванням - `keyword` та `http://127.0.0.1:8080`.

## Зупинка та оновлення

```sh
docker compose down
```

Документи й завантажена модель зберігаються. Для повторного запуску використовуйте `docker compose up -d`; після зміни коду - `docker compose up -d --build`. Індекс будується заново при запуску сервера.

## API

Базова адреса: `http://127.0.0.1:8080`. POST-запити та відповіді API використовують JSON.

| Метод | Маршрут | Призначення |
| --- | --- | --- |
| GET | `/` | Вебінтерфейс |
| GET | `/api/health` | Стан сервера й Ollama |
| GET | `/api/stats` | Статистика індексу |
| GET / POST | `/api/search` | Пошук документів |
| POST | `/api/index` | Перебудова індексу |
| WebSocket | `/ws/search` | Етапи пошуку з розширенням запиту |

<details>
<summary>Формати запитів, відповідей і WebSocket-повідомлень</summary>

### Стан і статистика

`GET /api/health`:

```json
{"status":"ok","ollama":true}
```

`status` - string, `ollama` - boolean. При недоступності моделі повертається HTTP `200` із `ollama: false`.

`GET /api/stats` (числові значення наведено для прикладу):

```json
{"documents":5,"index_build_time_ms":12.34,"threads_used":2}
```

`documents` і `threads_used` - integer; `index_build_time_ms` - number у мілісекундах.

### Пошук

`POST /api/search`, заголовок `Content-Type: application/json`:

```json
{"query":"computer","mode":"keyword"}
```

`query` - обов'язковий string, не лише пробільні символи, максимум 1000 байтів. `mode` - необов'язковий string: `keyword` або `llm`, за замовчуванням `keyword`.

Альтернатива: `GET /api/search?q=computer&mode=keyword`. Параметр `q` обов'язковий; для кількох слів використовуйте URL-кодування, наприклад `q=parallel%20search`.

Приклад відповіді для одного знайденого документа:

```json
{
  "query": "computer",
  "mode": "keyword",
  "expanded_query": "computer",
  "time_ms": 0.42,
  "total": 1,
  "results": [
    {
      "doc_id": "3",
      "filename": "04.txt",
      "score": 1,
      "matched_terms": ["computer"],
      "snippet": "Computer parallel computing."
    }
  ]
}
```

`query`, `mode`, `expanded_query` - string; `time_ms` - number; `total` - integer; `results` - array of objects. У результаті `doc_id`, `filename`, `snippet` - string, `score` - integer, `matched_terms` - array of strings.

Пошук працює за правилом OR. Оцінка `score` дорівнює кількості різних знайдених термінів; результати сортуються за спаданням оцінки, а при однаковій оцінці - за числовим ID. `snippet` містить початок документа. Без збігів повертаються HTTP `200`, `total: 0` та `results: []`. У режимі `llm` поле `expanded_query` містить початковий запит із доданими термінами.

### Перебудова індексу

`POST /api/index`, заголовок `Content-Type: application/json`:

```json
{"threads":4}
```

`threads` - обов'язковий integer від 1 до 128. Відповідь надходить після завершення побудови:

```json
{"status":"ok","documents_indexed":5,"time_ms":8.12}
```

`status` - string, `documents_indexed` - integer, `time_ms` - number. Поки будується новий індекс, пошук використовує попередній.

### Помилки REST

Формат помилок контролера: `{"error":"повідомлення"}`, де `error` - string.

| HTTP-код | Причина |
| --- | --- |
| 400 | Некоректний JSON або параметри |
| 409 | Перебудова індексу вже триває |
| 500 | Внутрішня помилка пошуку або перебудови |
| 503 | Розширення запиту через Ollama недоступне |

### WebSocket

Підключення: `ws://127.0.0.1:8080/ws/search`. Маршрут виконує LLM-пошук. Надішліть текстове JSON-повідомлення з обов'язковим рядковим `query` за тими самими обмеженнями довжини:

```json
{"query":"computer"}
```

Успішний пошук повертає три повідомлення; додаткові терміни залежать від моделі:

```json
{"stage":"query_expansion","data":{"original":"computer","expanded_terms":["computing","hardware"]}}
```

```json
{"stage":"keyword_search","data":{"results":[],"time_ms":0.42}}
```

```json
{"stage":"done"}
```

`stage` і `data.original` - string; `data.expanded_terms` - array of strings; `data.results` має формат результатів REST-пошуку; `data.time_ms` - number. Порожній `results` у прикладі означає відсутність збігів.

Усі часові значення наведено в мілісекундах. REST-показник `time_ms` у режимі `llm` включає звернення до Ollama; WebSocket-показник `data.time_ms` вимірює лише подальший пошук.

Помилка передається як `{"stage":"error","data":{"error":"повідомлення"}}`; `data.error` - string. Бінарні повідомлення не підтримуються. Для одного з'єднання дозволено один активний пошук, загалом - до чотирьох WebSocket-завдань. Після `done` з'єднання залишається відкритим для наступного запиту.

</details>

## Тести та графіки

Скрипти розташовані в `scripts/tests/`. Вимірювання зберігаються в `results/`, графіки - у `results/plots/`.

- `index-benchmark.ps1` - порівняння часу побудови індексу для різної кількості потоків.
- `search-load-test.py` - навантажувальне тестування пошуку.
- `plot-index-benchmark.py` і `plot-search-load.py` - побудова графіків із CSV за допомогою Matplotlib.

Перед вимірюваннями задайте очікувану кількість документів у тестових скриптах; перед побудовою графіків - шляхи `RAW_PATH` і `SUMMARY_PATH` до своїх CSV. Python потрібен лише для Python-скриптів.
