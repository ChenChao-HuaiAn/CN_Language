# GCC错误详细分类分析脚本
$ErrorActionPreference = "Continue"
$inputFile = "tools/gcc_errors_raw.txt"
$lines = Get-Content $inputFile -Encoding utf8 | Where-Object { $_ -match "error:" }

$totalErrors = $lines.Count
Write-Host "=== 总错误数: $totalErrors ==="

# 1. 按文件统计
Write-Host ""
Write-Host "=== 1. 按文件统计错误数（降序） ==="
$fileCounts = @{}
foreach ($line in $lines) {
    $file = ($line -split "\|")[0]
    if (-not $fileCounts.ContainsKey($file)) { $fileCounts[$file] = 0 }
    $fileCounts[$file]++
}
$fileCounts.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | ForEach-Object {
    $pct = [math]::Round($_.Value / $totalErrors * 100, 1)
    Write-Host "  $($_.Value)`t$($_.Key) ($pct%)"
}

# 2. 按错误类型分类
Write-Host ""
Write-Host "=== 2. 按错误类型详细分类 ==="
$categories = @{}
$categoryExamples = @{}

foreach ($line in $lines) {
    $errMsg = ($line -split "\|", 2)[1]
    
    $category = "Z-其他"
    
    if ($errMsg -match "implicit declaration of function") {
        $category = "I-隐式函数声明"
    } elseif ($errMsg -match "expected expression before") {
        $category = "L-表达式语法错误(expected expression)"
    } elseif ($errMsg -match "expected expression before") {
        $category = "L-表达式语法错误"
    } elseif ($errMsg -match "incompatible types when assigning to type 'struct") {
        $category = "C1-结构体值赋值不兼容(值vs指针)"
    } elseif ($errMsg -match "incompatible types when assigning to type 'int' from type 'struct") {
        $category = "C2-整数与结构体赋值不兼容"
    } elseif ($errMsg -match "incompatible types when returning") {
        $category = "C3-返回类型不兼容"
    } elseif ($errMsg -match "incompatible type for argument") {
        $category = "H1-函数参数类型不兼容"
    } elseif ($errMsg -match "incompatible pointer type") {
        $category = "H2-不兼容指针类型"
    } elseif ($errMsg -match "conflicting types for") {
        $category = "B-函数类型冲突"
    } elseif ($errMsg -match "makes integer from pointer without a cast") {
        $category = "D1-指针赋给整数(无转换)"
    } elseif ($errMsg -match "makes pointer from integer without a cast") {
        $category = "D2-整数赋给指针(无转换)"
    } elseif ($errMsg -match "undeclared \(first use") {
        $category = "J-变量未声明"
    } elseif ($errMsg -match "is a pointer; did you mean to use") {
        $category = "O1-指针用.而非->访问成员"
    } elseif ($errMsg -match "has no member named") {
        $category = "O2-结构体无此成员"
    } elseif ($errMsg -match "request for member.*in something not a structure") {
        $category = "O3-非结构体访问成员"
    } elseif ($errMsg -match "invalid type argument of unary") {
        $category = "E1-对非指针解引用(*)"
    } elseif ($errMsg -match "invalid use of void expression") {
        $category = "E2-无效使用void表达式"
    } elseif ($errMsg -match "invalid operands to binary") {
        $category = "E3-无效二元运算操作数"
    } elseif ($errMsg -match "too few arguments to function") {
        $category = "Q1-函数参数过少"
    } elseif ($errMsg -match "too (few|many) arguments") {
        $category = "Q-函数参数数量错误"
    } elseif ($errMsg -match "label.*used but not defined") {
        $category = "K-标签未定义"
    } elseif ($errMsg -match "passing argument.*makes pointer from integer") {
        $category = "D2-整数赋给指针(无转换)"
    } elseif ($errMsg -match "passing argument.*from incompatible") {
        $category = "H1-函数参数类型不兼容"
    } elseif ($errMsg -match "assignment to 'char \*' from 'char'") {
        $category = "D3-char*与char混淆"
    } elseif ($errMsg -match "assignment to 'char' from 'char \*'") {
        $category = "D3-char*与char混淆"
    } elseif ($errMsg -match "assignment to 'int \*' from incompatible") {
        $category = "H2-不兼容指针类型"
    } elseif ($errMsg -match "assignment to 'long long int' from") {
        $category = "D1-指针赋给整数(无转换)"
    } elseif ($errMsg -match "assignment to.*from 'int'") {
        $category = "D2-整数赋给指针(无转换)"
    } elseif ($errMsg -match "initialization of.*from") {
        $category = "D1-指针赋给整数(无转换)"
    } elseif ($errMsg -match "passing argument.*makes pointer from integer") {
        $category = "D2-整数赋给指针(无转换)"
    }
    
    if (-not $categories.ContainsKey($category)) { $categories[$category] = 0 }
    $categories[$category]++
    
    if (-not $categoryExamples.ContainsKey($category)) { $categoryExamples[$category] = @() }
    if ($categoryExamples[$category].Count -lt 3) {
        $categoryExamples[$category] += $errMsg
    }
}

$categories.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | ForEach-Object {
    $pct = [math]::Round($_.Value / $totalErrors * 100, 1)
    Write-Host "  $($_.Key): $($_.Value) ($pct%)"
}

# 3. 每个错误类型的具体示例
Write-Host ""
Write-Host "=== 3. 每个错误类型的具体示例 ==="
$categories.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | ForEach-Object {
    Write-Host ""
    Write-Host "--- $($_.Key) ($($_.Value)个) ---"
    foreach ($ex in $categoryExamples[$_.Key]) {
        # 截断过长的示例
        if ($ex.Length -gt 200) { $ex = $ex.Substring(0, 200) + "..." }
        Write-Host "  $ex"
    }
}

# 4. 隐式函数声明详细分析
Write-Host ""
Write-Host "=== 4. 隐式函数声明涉及的函数名列表 ==="
$implicitFunctions = @{}
$implicitByFile = @{}

foreach ($line in $lines) {
    $errMsg = ($line -split "\|", 2)[1]
    $file = ($line -split "\|")[0]
    if ($errMsg -match "implicit declaration of function '([^']+)'") {
        $funcName = $Matches[1]
        if (-not $implicitFunctions.ContainsKey($funcName)) { $implicitFunctions[$funcName] = 0 }
        $implicitFunctions[$funcName]++
        
        if (-not $implicitByFile.ContainsKey($funcName)) { $implicitByFile[$funcName] = @() }
        if ($implicitByFile[$funcName] -notcontains $file) {
            $implicitByFile[$funcName] += $file
        }
    }
}

Write-Host "隐式声明的函数共 $($implicitFunctions.Count) 个："
$implicitFunctions.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | ForEach-Object {
    $files = $implicitByFile[$_.Key] -join ", "
    Write-Host "  $($_.Key) ($($_.Value)次) - 出现在: $files"
}

# 5. 变量未声明详细分析
Write-Host ""
Write-Host "=== 5. 变量未声明详细分析 ==="
$undeclaredVars = @{}
foreach ($line in $lines) {
    $errMsg = ($line -split "\|", 2)[1]
    $file = ($line -split "\|")[0]
    if ($errMsg -match "'(cn_var_[^']+)' undeclared") {
        $varName = $Matches[1]
        if (-not $undeclaredVars.ContainsKey($varName)) { $undeclaredVars[$varName] = @() }
        if ($undeclaredVars[$varName] -notcontains $file) {
            $undeclaredVars[$varName] += $file
        }
    }
}

Write-Host "未声明的变量共 $($undeclaredVars.Count) 个："
$undeclaredVars.GetEnumerator() | ForEach-Object {
    $files = $_.Value -join ", "
    Write-Host "  $($_.Key) - 出现在: $files"
}

# 6. 函数类型冲突详细分析
Write-Host ""
Write-Host "=== 6. 函数类型冲突详细分析 ==="
foreach ($line in $lines) {
    $errMsg = ($line -split "\|", 2)[1]
    if ($errMsg -match "conflicting types for") {
        Write-Host "  $errMsg"
    }
}

# 7. 结构体成员错误详细分析
Write-Host ""
Write-Host "=== 7. 结构体成员错误详细分析 ==="
foreach ($line in $lines) {
    $errMsg = ($line -split "\|", 2)[1]
    if ($errMsg -match "has no member named|request for member") {
        Write-Host "  $errMsg"
    }
}

Write-Host ""
Write-Host "=== 分析完成 ==="
