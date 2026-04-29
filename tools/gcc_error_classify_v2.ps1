# P0-2修复验证：精细错误分类分析
$ErrorActionPreference = "Continue"
$baseDir = "c:\Users\ChenChao\Documents\gitcode\CN_Language"
Set-Location $baseDir

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

# 精细分类
$categories = @{}
foreach ($e in $allErrors) {
    $err = $e.Error
    $cat = "Z-其他"
    
    if ($err -match "unknown type name") { $cat = "A1-未知类型名(CN类型未翻译)" }
    elseif ($err -match "expected '=', ',', ';', 'asm' or '__attribute__' before '\{' token") { $cat = "A2-函数声明语法错误(类型名级联)" }
    elseif ($err -match "expected '=', ',', ';', 'asm' or '__attribute__' before '->'") { $cat = "A3-成员访问语法错误(类型名级联)" }
    elseif ($err -match "expected declaration specifiers before") { $cat = "A4-声明语法错误(类型名级联)" }
    elseif ($err -match "expected '=', ',', ';', 'asm' or '__attribute__' before") { $cat = "A5-声明语法错误(其他)" }
    elseif ($err -match "conflicting types") { $cat = "B-函数类型冲突(conflicting types)" }
    elseif ($err -match "assignment to 'struct.*\*' from incompatible pointer type 'struct") { $cat = "C1-结构体指针类型不兼容赋值" }
    elseif ($err -match "assignment to 'long long int' from 'struct") { $cat = "C2-结构体指针赋值给整数" }
    elseif ($err -match "assignment to 'long long int' from 'char \*'") { $cat = "C3-字符串指针赋值给整数" }
    elseif ($err -match "assignment to 'long long int' from") { $cat = "C4-其他整数赋值类型错误" }
    elseif ($err -match "assignment to 'char \*' from 'char'") { $cat = "D1-char*<-char(字符串/字符混淆)" }
    elseif ($err -match "assignment to 'char' from 'char \*'") { $cat = "D2-char<-char*(字符串/字符混淆)" }
    elseif ($err -match "invalid type argument of unary '\*'.*have 'char'") { $cat = "D3-解引用char类型" }
    elseif ($err -match "assignment to 'char \*' from 'int'") { $cat = "D4-char*<-int(隐式声明级联)" }
    elseif ($err -match "assignment to 'void \*' from 'int'") { $cat = "D5-void*<-int(隐式声明级联)" }
    elseif ($err -match "assignment to 'int \*' from incompati") { $cat = "E-指针类型不兼容赋值" }
    elseif ($err -match "incompatible types when assigning") { $cat = "F-不兼容类型赋值(其他)" }
    elseif ($err -match "incompatible type for argument") { $cat = "G1-函数参数类型不兼容" }
    elseif ($err -match "passing argument.*incompati") { $cat = "G2-函数参数传递不兼容" }
    elseif ($err -match "too few arguments") { $cat = "H-函数参数不足" }
    elseif ($err -match "implicit declaration") { $cat = "I-隐式函数声明" }
    elseif ($err -match "has no member named") { $cat = "J1-结构体无此成员" }
    elseif ($err -match "request for member.*not a structure") { $cat = "J2-非结构体访问成员" }
    elseif ($err -match "invalid use of void expression") { $cat = "K-void表达式使用错误" }
    elseif ($err -match "expected expression before ';'") { $cat = "L1-空表达式(分号前缺表达式)" }
    elseif ($err -match "expected expression before") { $cat = "L2-表达式语法错误(其他)" }
    elseif ($err -match "expected '\)' before") { $cat = "L3-括号不匹配" }
    elseif ($err -match "invalid operands to binary") { $cat = "M-二元运算符操作数无效" }
    elseif ($err -match "returning.*from.*incompati") { $cat = "N-返回类型不兼容" }
    elseif ($err -match "undeclared|was not declared") { $cat = "O-变量未声明" }
    elseif ($err -match "initialization of.*from") { $cat = "P-初始化类型不匹配" }
    elseif ($err -match "label.*used but not defined") { $cat = "Q-goto标签未定义" }
    elseif ($err -match "old-style parameter") { $cat = "R-旧式参数声明" }
    elseif ($err -match "expected '\{' at end of input") { $cat = "S-文件末尾缺大括号" }
    elseif ($err -match "dereferencing pointer to incompatible") { $cat = "T-指针解引用类型不兼容" }
    
    if (-not $categories.ContainsKey($cat)) { $categories[$cat] = 0 }
    $categories[$cat]++
}

Write-Output "=== 精细错误分类统计 ==="
$categories.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | ForEach-Object {
    Write-Output "$($_.Value.ToString().PadLeft(5)) | $($_.Key)"
}

Write-Output ""
Write-Output "=== 错误大类汇总 ==="
$majorCats = @{}
foreach ($e in $allErrors) {
    $err = $e.Error
    $cat = "Z-其他"
    
    if ($err -match "unknown type name|expected '=', ',', ';', 'asm' or '__attribute__' before '\{'|expected '=', ',', ';', 'asm' or '__attribute__' before '->'|expected declaration specifiers before|expected '=', ',', ';', 'asm' or '__attribute__' before") { $cat = "A-CN类型名未翻译(级联错误)" }
    elseif ($err -match "conflicting types") { $cat = "B-函数类型冲突" }
    elseif ($err -match "assignment.*incompatible pointer type.*struct|assignment to 'struct.*\*' from incompatible") { $cat = "C-结构体指针类型不兼容" }
    elseif ($err -match "assignment to 'long long int' from") { $cat = "D-整数赋值类型错误" }
    elseif ($err -match "assignment to 'char \*' from 'char'|assignment to 'char' from 'char \*'|invalid type argument of unary '\*'.*have 'char'") { $cat = "E-字符/字符串类型混淆" }
    elseif ($err -match "assignment to 'char \*' from 'int'|assignment to 'void \*' from 'int'") { $cat = "F-隐式声明级联赋值错误" }
    elseif ($err -match "incompatible types when assigning|assignment to 'int \*' from incompati") { $cat = "G-其他赋值类型不兼容" }
    elseif ($err -match "incompatible type for argument|passing argument.*incompati|too few arguments") { $cat = "H-函数参数错误" }
    elseif ($err -match "implicit declaration") { $cat = "I-隐式函数声明" }
    elseif ($err -match "has no member named|request for member.*not a structure") { $cat = "J-结构体成员访问错误" }
    elseif ($err -match "invalid use of void expression") { $cat = "K-void表达式错误" }
    elseif ($err -match "expected expression before|expected '\)' before") { $cat = "L-表达式语法错误" }
    elseif ($err -match "invalid operands to binary") { $cat = "M-运算符错误" }
    elseif ($err -match "returning.*from.*incompati") { $cat = "N-返回类型不兼容" }
    elseif ($err -match "undeclared|was not declared") { $cat = "O-变量未声明" }
    elseif ($err -match "initialization of.*from") { $cat = "P-初始化不匹配" }
    elseif ($err -match "label.*used but not defined|old-style parameter|expected '\{' at end") { $cat = "Q-其他语法错误" }
    elseif ($err -match "dereferencing pointer to incompatible") { $cat = "R-指针解引用不兼容" }
    
    if (-not $majorCats.ContainsKey($cat)) { $majorCats[$cat] = 0 }
    $majorCats[$cat]++
}

$majorCats.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | ForEach-Object {
    Write-Output "$($_.Value.ToString().PadLeft(5)) | $($_.Key)"
}
