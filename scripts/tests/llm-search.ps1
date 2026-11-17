$body = @{
    query = "computer"
    mode = "llm"
} | ConvertTo-Json

$response = Invoke-RestMethod `
    -Uri "http://127.0.0.1:8080/api/search" `
    -Method Post `
    -ContentType "application/json" `
    -Body $body `
    -TimeoutSec 150

$response |
    Select-Object query, mode, expanded_query, total, time_ms |
    Format-List

$response.results |
    Select-Object filename, score, matched_terms, snippet |
    Format-Table -AutoSize

$document = $response.results |
    Where-Object { $_.filename -eq "04.txt" } |
    Select-Object -First 1

$checks = @(
    [PSCustomObject]@{
        Test = "LLM mode"
        Passed = ($response.mode -eq "llm")
    }
    [PSCustomObject]@{
        Test = "Original term preserved"
        Passed = ($response.expanded_query -match '\bcomputer\b')
    }
    [PSCustomObject]@{
        Test = "Document 04.txt found"
        Passed = ($null -ne $document)
    }
    [PSCustomObject]@{
        Test = "Original term matched"
        Passed = ($null -ne $document -and
            $document.matched_terms -contains "computer")
    }
)

$checks | Format-Table -AutoSize

if ($checks.Passed -contains $false) {
    throw "LLM search checks failed."
}