param(
    [int]$ExpectedDocuments = 2770
)

$ErrorActionPreference = "Stop"

$threadCounts = @(1, 2, 4, 6, 8, 12)
$repetitions = 5

$projectDirectory = (
    Resolve-Path (Join-Path $PSScriptRoot "../..")
).Path

$sourceDirectory = Join-Path $projectDirectory "src"
$testDirectory = $PSScriptRoot
$outputDirectory = Join-Path $projectDirectory "results"

New-Item -ItemType Directory `
    -Force `
    -Path $outputDirectory | Out-Null

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"

$rawPath = Join-Path `
    $outputDirectory "index-process-raw-$stamp.csv"

$summaryPath = Join-Path `
    $outputDirectory "index-process-summary-$stamp.csv"

function Invoke-IndexBenchmark {
    param([int]$Threads)

    # Новий контейнер і новий процес для кожної побудови.
    $output = & docker compose run `
        --rm `
        --no-deps `
        -T `
        --volume "${outputDirectory}:/bench:ro" `
        --entrypoint /bench/index_benchmark `
        server `
        /app/data `
        "$Threads"

    if ($LASTEXITCODE -ne 0) {
        throw "Benchmark failed for threads=$Threads."
    }

    $result = ($output -join "`n") | ConvertFrom-Json

    if (
        $result.threads -ne $Threads -or
        $result.documents -ne $ExpectedDocuments -or
        $null -eq $result.time_ms -or
        [double]$result.time_ms -le 0
    ) {
        throw "Invalid benchmark result for threads=$Threads."
    }

    return $result
}

Push-Location $projectDirectory

try {
    Write-Host "Compiling benchmark..."

    # Використовуємо поточні вихідні файли з репозиторію.
    & docker compose run `
        --rm `
        --no-deps `
        -T `
        --volume "${sourceDirectory}:/source:ro" `
        --volume "${testDirectory}:/tests:ro" `
        --volume "${outputDirectory}:/output" `
        --entrypoint g++ `
        server `
        -std=c++20 `
        -O2 `
        -DNDEBUG `
        -pthread `
        -I/source `
        /tests/index_benchmark.cpp `
        /source/DataLoader.cpp `
        /source/TextProcessor.cpp `
        /source/InvertedIndex.cpp `
        /source/Indexer.cpp `
        /source/ThreadPool.cpp `
        -o /output/index_benchmark

    if ($LASTEXITCODE -ne 0) {
        throw "Benchmark compilation failed."
    }

    Write-Host "Warm-up rebuild..."
    $warmup = Invoke-IndexBenchmark -Threads 1

    Write-Host (
        "Warm-up completed: {0:N2} ms; documents={1}" -f `
            $warmup.time_ms, $warmup.documents
    )

    $measurements = @()

    foreach ($run in 1..$repetitions) {
        $order = @(
            $threadCounts |
                Get-Random -Count $threadCounts.Count
        )

        foreach ($threads in $order) {
            Write-Host (
                "Run {0}/{1}; threads={2}" -f `
                    $run, $repetitions, $threads
            )

            $result = Invoke-IndexBenchmark -Threads $threads

            $row = [PSCustomObject]@{
                Run       = $run
                Threads   = $threads
                Documents = [long]$result.documents
                TimeMs    = [double]$result.time_ms
            }

            $measurements += $row

            # Зберігаємо результат одразу після вимірювання.
            $row | Export-Csv `
                -Path $rawPath `
                -NoTypeInformation `
                -Encoding UTF8 `
                -Append

            Write-Host (
                "Completed: {0:N2} ms" -f $row.TimeMs
            )
        }
    }

    $summary = @(
        foreach ($threads in $threadCounts) {
            $values = @(
                $measurements |
                    Where-Object { $_.Threads -eq $threads } |
                    ForEach-Object { $_.TimeMs }
            )

            $stats = $values |
                Measure-Object -Average -Minimum -Maximum

            [PSCustomObject]@{
                Threads = $threads
                Runs    = $values.Count
                MeanMs  = [double]$stats.Average
                MinMs   = [double]$stats.Minimum
                MaxMs   = [double]$stats.Maximum
            }
        }
    )

    $singleThreadTime = (
        $summary | Where-Object { $_.Threads -eq 1 }
    ).MeanMs

    $final = @(
        foreach ($row in $summary) {
            $speedup = $singleThreadTime / $row.MeanMs

            [PSCustomObject]@{
                Threads = $row.Threads
                Runs = $row.Runs

                MeanMs = [math]::Round($row.MeanMs, 2)
                MinMs = [math]::Round($row.MinMs, 2)
                MaxMs = [math]::Round($row.MaxMs, 2)

                Speedup = [math]::Round($speedup, 3)

                EfficiencyPercent = [math]::Round(
                    100 * $speedup / $row.Threads, 2
                )
            }
        }
    )

    $final | Export-Csv `
        -Path $summaryPath `
        -NoTypeInformation `
        -Encoding UTF8

    $final | Format-Table -AutoSize

    Write-Host "Raw measurements: $rawPath"
    Write-Host "Summary: $summaryPath"
}
finally {
    Pop-Location
}