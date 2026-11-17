function Test-ApiStatus {
    param(
        [string]$Name,
        [string]$Path,
        [string]$Body,
        [int]$Expected
    )

    try {
        $response = Invoke-WebRequest `
            -Uri "http://127.0.0.1:8080$Path" `
            -Method Post `
            -ContentType "application/json" `
            -Body $Body `
            -UseBasicParsing `
            -ErrorAction Stop

        $actual = [int]$response.StatusCode
    }
    catch {
        if ($null -eq $_.Exception.Response) {
            throw
        }

        $actual = [int]$_.Exception.Response.StatusCode
    }

    [PSCustomObject]@{
        Test     = $Name
        Expected = $Expected
        Actual   = $actual
        Passed   = ($actual -eq $Expected)
    }
}


$checks = @(
    Test-ApiStatus "Missing query" "/api/search" '{}' 400

    Test-ApiStatus "Empty query" "/api/search" '{"query":""}' 400

    Test-ApiStatus "Whitespace query" "/api/search" '{"query":"   "}' 400

    Test-ApiStatus "Numeric query" "/api/search" '{"query":42}' 400

    Test-ApiStatus "Unknown mode" "/api/search" '{"query":"cat","mode":"unknown"}' 400

    Test-ApiStatus "Invalid JSON" "/api/search" '{"query":' 400

    Test-ApiStatus "Default keyword mode" "/api/search" '{"query":"cat"}' 200

    Test-ApiStatus "Missing threads" "/api/index" '{}' 400

    Test-ApiStatus "Zero threads" "/api/index" '{"threads":0}' 400

    Test-ApiStatus "Too many threads" "/api/index" '{"threads":129}' 400

    Test-ApiStatus "String threads" "/api/index" '{"threads":"4"}' 400

    Test-ApiStatus "Fractional threads" "/api/index" '{"threads":1.5}' 400
)

$checks | Format-Table -AutoSize

$body1000 = @{ query = ("z" * 1000) } | ConvertTo-Json
$body1001 = @{ query = ("z" * 1001) } | ConvertTo-Json

Test-ApiStatus "1000 bytes" "/api/search" $body1000 200
Test-ApiStatus "1001 bytes" "/api/search" $body1001 400