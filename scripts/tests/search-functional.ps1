function Test-Search {
    param([string]$Query)

    $body = @{
        query = $Query
        mode = "keyword"
    } | ConvertTo-Json

    $response = Invoke-RestMethod `
        -Uri "http://127.0.0.1:8080/api/search" `
        -Method Post `
        -ContentType "application/json" `
        -Body $body

    Write-Host "Query: $Query | Total: $($response.total)"

    $response.results |
        Select-Object filename, score, matched_terms |
        Format-Table -AutoSize
}

Test-Search "cat"
Test-Search "cat dog"
Test-Search "CAT!!!"
Test-Search "cat cat dog"
Test-Search "bird"
Test-Search "123"
Test-Search "elephant"
Test-Search "!!!"
