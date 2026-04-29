# P0-2修复验证：详细错误分类分析
$ErrorActionPreference = "Continue"
$baseDir = "c:\Users\ChenChao\Documents\gitcode\CN_Language"
Set-Location $baseDir

# 获取所有C文件的GCC错误
$files = Get-ChildItem -Path src_cn -Filter '*.c' -Recurse | Sort-Object FullName
$allErrors = @()

foreach ($f in $files) {
    $rel = $f.FullName.Replace("$baseDir\", "")
    $output = & gcc -c -I include -I src_cn $f.FullName -o NUL 2>&1 | Out-String
    $errorLines = $output -split "`n" | Select-String "error:"
    foreach ($line in $errorLines) {
        $allErrors += [PSCustomObject]@{File=$rel; Error=$line.Line.Trim()}
    }
}

Write-Output "=== 总错误数: $($allErrors.Count) ==="
Write-Output ""

# 分类统计
$categories = @{}
foreach ($e in $allErrors) {
    $err = $e.Error
    $cat = "其他"
    
    if ($err -match "conflicting types") { $cat = "A-函数类型冲突(conflicting types)" }
    elseif ($err -match "assignment to 'char \*' from 'char'") { $cat = "B-字符/字符串类型混淆(char*/char)" }
    elseif ($err -match "assignment to 'char' from 'char \*'") { $cat = "B-字符/字符串类型混淆(char/char*)" }
    elseif ($err -match "invalid type argument of unary '\*'") { $cat = "B-字符/字符串类型混淆(解引用char)" }
    elseif ($err -match "assignment to 'long long int' from 'struct") { $cat = "C-结构体赋值给整数类型" }
    elseif ($err -match "assignment to 'long long int' from 'char \*'") { $cat = "D-字符串赋值给整数类型" }
    elseif ($err -match "assignment to 'int \*' from incompati") { $cat = "E-指针类型不兼容赋值" }
    elseif ($err -match "incompatible types when assigning") { $cat = "F-不兼容类型赋值" }
    elseif ($err -match "incompatible type for argument") { $cat = "G-函数参数类型不兼容" }
    elseif ($err -match "passing argument.*incompati") { $cat = "G-函数参数类型不兼容" }
    elseif ($err -match "too few arguments") { $cat = "H-函数参数不足" }
    elseif ($err -match "implicit declaration") { $cat = "I-隐式函数声明" }
    elseif ($err -match "request for member") { $cat = "J-结构体成员访问错误" }
    elseif ($err -match "'struct.*has no member") { $cat = "J-结构体成员访问错误" }
    elseif ($err -match "dereferencing pointer to incompatible") { $cat = "K-指针解引用类型不兼容" }
    elseif ($err -match "invalid use of void expression") { $cat = "L-void表达式使用错误" }
    elseif ($err -match "expected expression before") { $cat = "M-表达式语法错误" }
    elseif ($err -match "invalid operands to binary") { $cat = "N-二元运算符操作数无效" }
    elseif ($err -match "returning.*from.*incompati") { $cat = "O-返回类型不兼容" }
    elseif ($err -match "undeclared|was not declared") { $cat = "P-变量未声明" }
    elseif ($err -match "initialization of.*from") { $cat = "Q-初始化类型不匹配" }
    elseif ($err -match "assignment to 'void \*' from 'int'") { $cat = "R-void指针赋值错误" }
    elseif ($err -match "assignment to 'char \*' from 'int'") { $cat = "S-字符指针从int赋值" }
    
    if (-not $categories.ContainsKey($cat)) { $categories[$cat] = 0 }
    $categories[$cat]++
}

Write-Output "=== 错误分类统计 ==="
$categories.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | ForEach-Object {
    Write-Output "$($_.Value) | $($_.Key)"
}

Write-Output ""
Write-Output "=== 各文件错误分类 ==="
$fileCats = @{}
foreach ($e in $allErrors) {
    $err = $e.Error
    $cat = "其他"
    
    if ($err -match "conflicting types") { $cat = "A-函数类型冲突" }
    elseif ($err -match "assignment to 'char \*' from 'char'") { $cat = "B-char*/char混淆" }
    elseif ($err -match "assignment to 'char' from 'char \*'") { $cat = "B-char/char*混淆" }
    elseif ($err -match "invalid type argument of unary '\*'") { $cat = "B-解引用char" }
    elseif ($err -match "assignment to 'long long int' from 'struct") { $cat = "C-结构体赋整数" }
    elseif ($err -match "assignment to 'long long int' from 'char \*'") { $cat = "D-字符串赋整数" }
    elseif ($err -match "assignment to 'int \*' from incompati") { $cat = "E-指针赋值不兼容" }
    elseif ($err -match "incompatible types when assigning") { $cat = "F-赋值类型不兼容" }
    elseif ($err -match "incompatible type for argument") { $cat = "G-参数类型不兼容" }
    elseif ($err -match "passing argument.*incompati") { $cat = "G-参数类型不兼容" }
    elseif ($err -match "too few arguments") { $cat = "H-参数不足" }
    elseif ($err -match "implicit declaration") { $cat = "I-隐式声明" }
    elseif ($err -match "request for member") { $cat = "J-成员访问错误" }
    elseif ($err -match "'struct.*has no member") { $cat = "J-成员访问错误" }
    elseif ($err -match "invalid use of void expression") { $cat = "L-void表达式" }
    elseif ($err -match "expected expression before") { $cat = "M-表达式语法" }
    elseif ($err -match "invalid operands to binary") { $cat = "N-运算符无效" }
    elseif ($err -match "returning.*from.*incompati") { $cat = "O-返回不兼容" }
    elseif ($err -match "undeclared|was not declared") { $cat = "P-未声明" }
    elseif ($err -match "initialization of.*from") { $cat = "Q-初始化不匹配" }
    elseif ($err -match "assignment to 'void \*' from 'int'") { $cat = "R-void*赋值" }
    elseif ($err -match "assignment to 'char \*' from 'int'") { $cat = "S-char*=int" }
    
    $key = "$($e.File)|$cat"
    if (-not $fileCats.ContainsKey($key)) { $fileCats[$key] = 0 }
    $fileCats[$key]++
}

# 按文件输出
$fileGroups = $fileCats.GetEnumerator() | Group-Object { $_.Key.Split('|')[0] } | Sort-Object Name
foreach ($fg in $fileGroups) {
    $fileName = $fg.Name
    $cats = $fg.Group | Sort-Object { [int]$_.Value } -Descending
    $totalForFile = ($cats | Measure-Object { $_.Value } -Sum).Sum
    Write-Output "--- $fileName (总错误: $totalForFile) ---"
    foreach ($c in $cats) {
        $catName = $c.Key.Split('|')[1]
        Write-Output "  $($c.Value) | $catName"
    }
    Write-Output ""
}
