# 分析GCC错误分类
$errors = Get-Content 'gcc_errors_only.txt' -Encoding utf8 | Select-String 'error:'
$typeMap = @{}

foreach ($e in $errors) {
    $line = $e.ToString()
    if ($line -match 'error: (.+?)$') {
        $msg = $matches[1].Trim()
        
        # 归类
        if ($msg -match 'too many arguments') {
            $cat = 'A-too many arguments'
        } elseif ($msg -match 'too few arguments') {
            $cat = 'B-too few arguments'
        } elseif ($msg -match 'incompatible types when assign') {
            $cat = 'C-incompatible types (assignment)'
        } elseif ($msg -match 'incompatible type for argument') {
            $cat = 'D-incompatible type (argument)'
        } elseif ($msg -match 'incompatible types when return') {
            $cat = 'E-incompatible types (return)'
        } elseif ($msg -match 'assignment to .+ from') {
            $cat = 'F-assignment type mismatch'
        } elseif ($msg -match 'initialization of .+ from') {
            $cat = 'G-initialization type mismatch'
        } elseif ($msg -match 'passing argument .+ of') {
            $cat = 'H-passing argument type mismatch'
        } elseif ($msg -match 'conflicting types') {
            $cat = 'I-conflicting types'
        } elseif ($msg -match 'request for member') {
            $cat = 'J-request for member (dot vs arrow)'
        } elseif ($msg -match 'invalid operands to binary') {
            $cat = 'K-invalid operands (string/ptr math)'
        } elseif ($msg -match 'invalid type argument of unary') {
            $cat = 'L-invalid dereference (non-pointer)'
        } elseif ($msg -match 'invalid use of void') {
            $cat = 'M-invalid use of void expression'
        } elseif ($msg -match 'undeclared|undec') {
            $cat = 'N-undeclared variable'
        } elseif ($msg -match 'expected expression') {
            $cat = 'O-expected expression (syntax)'
        } elseif ($msg -match 'label.*used but not defined') {
            $cat = 'P-label not defined'
        } elseif ($msg -match 'struct.*has no member') {
            $cat = 'Q-struct has no member'
        } else {
            $cat = "Z-other: " + $msg.Substring(0, [Math]::Min(60, $msg.Length))
        }
        
        if (-not $typeMap.ContainsKey($cat)) {
            $typeMap[$cat] = 0
        }
        $typeMap[$cat]++
    }
}

Write-Host "=== 错误类型分类统计 ==="
$typeMap.GetEnumerator() | Sort-Object Name | Format-Table Name, Value -AutoSize
Write-Host ""
Write-Host "=== 总计 ==="
$total = ($typeMap.Values | Measure-Object -Sum).Sum
Write-Host "Total errors: $total"
