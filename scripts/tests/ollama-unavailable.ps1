$baseUrl = "http://127.0.0.1:8080"

function Send-Search {
    param([string]$Mode)

    Invoke-RestMethod `
        -Uri "$baseUrl/api/search" `
        -Method Post `
        -ContentType "application/json" `
        -Body (@{
            query = "computer"
            mode = $Mode
        } | ConvertTo-Json) `
        -TimeoutSec 150 `
        -ErrorAction Stop
}

# Контрольна відповідь до зупинки Ollama
$baseline = Send-Search "keyword"

try {
    docker compose stop ollama

    if ($LASTEXITCODE -ne 0) {
        throw "Could not stop Ollama."
    }

    $health = Invoke-RestMethod `
        -Uri "$baseUrl/api/health" `
        -ErrorAction Stop

    $keyword = Send-Search "keyword"

    $llmStatus = 200
    $llmError = ""

    try {
        $null = Send-Search "llm"
    }
    catch {
        if ($null -eq $_.Exception.Response) {
            throw
        }

        $llmStatus = [int]$_.Exception.Response.StatusCode
        $llmError = $_.ErrorDetails.Message
    }

    $checks = @(
        [PSCustomObject]@{
            Test = "Server works, Ollama unavailable"
            Passed = (
                $health.status -eq "ok" -and
                $health.ollama -eq $false
            )
        }
        [PSCustomObject]@{
            Test = "Keyword search remains available"
            Passed = (
                $keyword.mode -eq "keyword" -and
                $keyword.total -eq $baseline.total
            )
        }
        [PSCustomObject]@{
            Test = "LLM search returns 503"
            Passed = ($llmStatus -eq 503)
        }
    )

    $checks | Format-Table -AutoSize

    Write-Host "LLM error response:"
    Write-Host $llmError
}
finally {
    # Відновлюємо Ollama навіть після помилки тесту
    docker compose start ollama
}