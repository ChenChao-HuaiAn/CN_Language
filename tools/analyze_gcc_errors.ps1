# P0-3a修复后GCC错误分析脚本
# Step 1: 编译所有CN文件
# Step 2: GCC编译生成的C文件并收集错误

Write-Host "=== Step 1: 编译CN文件 ===" -ForegroundColor Green

$cnFiles = Get-ChildItem -Path "src_cn" -Filter "*.cn" -Recurse
$success = 0
$fail = 0
$failedFiles = @()

foreach ($f in $cnFiles) {
    $output = $f.FullName -replace '\.cn$', '.c'
    $result = & "build/bin/Release/cnc.exe" $f.FullName -o $output 2>&1
    if ($LASTEXITCODE -eq 0) {
        $success++
    } else {
        $fail++
        $failedFiles += $f.FullName
        Write-Host "FAIL: $($f.FullName)"
    }
}

Write-Host ""
Write-Host "=== CN编译结果 ==="
Write-Host "成功: $success"
Write-Host "失败: $fail"
Write-Host "总计: $($cnFiles.Count)"

# Step 2: GCC编译所有生成的C文件
Write-Host ""
Write-Host "=== Step 2: GCC编译C文件 ===" -ForegroundColor Green

$cFiles = Get-ChildItem -Path "src_cn" -Filter "*.c" -Recurse
$allErrors = @()
$fileErrorCounts = @{}

foreach ($f in $cFiles) {
    $gccOutput = & gcc -c -fsyntax-only $f.FullName 2>&1
    $errors = $gccOutput | Where-Object { $_ -match "error:" }
    
    if ($errors) {
        $relPath = $f.FullName.Replace((Get-Location).Path + "\", "")
        $fileErrorCounts[$relPath] = @($errors).Count
        
        foreach ($err in $errors) {
            $allErrors += "$relPath|$_"
        }
    }
}

Write-Host ""
Write-Host "=== Step 3: 按文件统计错误数（降序） ===" -ForegroundColor Green

$fileErrorCounts.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | ForEach-Object {
    Write-Host "$($_.Value)`t$($_.Key)"
}

$totalErrors = $allErrors.Count
Write-Host ""
Write-Host "总错误数: $totalErrors"

# Step 4: 按错误类型分类
Write-Host ""
Write-Host "=== Step 4: 按错误类型分类 ===" -ForegroundColor Green

$errorCategories = @{}

foreach ($errLine in $allErrors) {
    $parts = $errLine -split "\|", 2
    $file = $parts[0]
    $errMsg = $parts[1]
    
    # 分类错误
    $category = "Z-其他"
    
    if ($errMsg -match "implicit declaration of function") {
        $category = "I-隐式函数声明"
    } elseif ($errMsg -match "expected expression") {
        $category = "L-表达式语法错误"
    } elseif ($errMsg -match "incompatible type") {
        $category = "C-类型不兼容"
    } elseif ($errMsg -match "unknown type name") {
        $category = "A-未知类型名"
    } elseif ($errMsg -match "conflicting types") {
        $category = "B-函数类型冲突"
    } elseif ($errMsg -match "undeclared|not declared") {
        $category = "J-变量未声明"
    } elseif ($errMsg -match "expected.*before|expected.*at end") {
        $category = "K-语法期望错误"
    } elseif ($errMsg -match "invalid type|invalid use") {
        $category = "D-无效类型使用"
    } elseif ($errMsg -match "dereferencing|pointer type") {
        $category = "E-指针解引用错误"
    } elseif ($errMsg -match "subscripted|array type") {
        $category = "F-数组下标错误"
    } elseif ($errMsg -match "redefinition") {
        $category = "G-重定义错误"
    } elseif ($errMsg -match "passing .* to parameter|incompatible pointer type") {
        $category = "H-参数类型不匹配"
    } elseif ($errMsg -match "void function|non-void function") {
        $category = "M-返回值错误"
    } elseif ($errMsg -match "assignment.*from|assigning to") {
        $category = "N-赋值类型错误"
    } elseif ($errMsg -match "member reference|not a structure|has no member") {
        $category = "O-结构体成员错误"
    } elseif ($errMsg -match "called object|not a function") {
        $category = "P-非函数调用"
    } elseif ($errMsg -match "too (few|many) arguments") {
        $category = "Q-参数数量错误"
    }
    
    if (-not $errorCategories.ContainsKey($category)) {
        $errorCategories[$category] = @()
    }
    $errorCategories[$category] += $errMsg
}

$errorCategories.GetEnumerator() | Sort-Object { [int]$_.Value.Count } -Descending | ForEach-Object {
    $count = $_.Value.Count
    $pct = [math]::Round($count / $totalErrors * 100, 1)
    Write-Host "$($_.Key): $count ($pct%)"
}

# Step 5: 隐式函数声明详细分析
Write-Host ""
Write-Host "=== Step 5: 隐式函数声明详细分析 ===" -ForegroundColor Green

$implicitFunctions = @{}
foreach ($errLine in $allErrors) {
    $parts = $errLine -split "\|", 2
    $errMsg = $parts[1]
    if ($errMsg -match "implicit declaration of function '(\w+)'") {
        $funcName = $Matches[1]
        if (-not $implicitFunctions.ContainsKey($funcName)) {
            $implicitFunctions[$funcName] = 0
        }
        $implicitFunctions[$funcName]++
    }
}

Write-Host "隐式声明的函数列表（按出现次数降序）："
$implicitFunctions.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | ForEach-Object {
    Write-Host "  $($_.Key): $($_.Value)次"
}

# Step 6: 每个错误类型的具体示例
Write-Host ""
Write-Host "=== Step 6: 每个错误类型的具体示例 ===" -ForegroundColor Green

$errorCategories.GetEnumerator() | Sort-Object { [int]$_.Value.Count } -Descending | ForEach-Object {
    Write-Host ""
    Write-Host "--- $($_.Key) ---"
    # 输出前3个示例
    $examples = $_.Value | Select-Object -First 3
    foreach ($ex in $examples) {
        Write-Host "  $ex"
    }
}

Write-Host ""
Write-Host "=== 分析完成 ===" -ForegroundColor Green
