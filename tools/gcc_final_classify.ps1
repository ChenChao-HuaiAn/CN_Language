# 最终精细错误分类
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

# 最终精细分类
$categories = @{}
foreach ($e in $allErrors) {
    $err = $e.Error
    $cat = "Z-其他"
    
    # A类: CN类型名未翻译(级联错误)
    if ($err -match "unknown type name") { $cat = "A1-未知类型名(CN类型未翻译)" }
    elseif ($err -match "type defaults to 'int' in declaration") { $cat = "A2-类型默认int(CN类型未翻译级联)" }
    elseif ($err -match "expected '=', ',', ';', 'asm' or '__attribute__' before '\{' token") { $cat = "A3-函数声明语法错误(CN类型级联)" }
    elseif ($err -match "expected '=', ',', ';', 'asm' or '__attribute__' before '->'") { $cat = "A4-成员访问语法错误(CN类型级联)" }
    elseif ($err -match "expected declaration specifiers before") { $cat = "A5-声明语法错误(CN类型级联)" }
    elseif ($err -match "expected '=', ',', ';', 'asm' or '__attribute__' before") { $cat = "A6-其他声明语法错误(CN类型级联)" }
    elseif ($err -match "expected identifier or '\(' before") { $cat = "A7-标识符语法错误(CN类型级联)" }
    elseif ($err -match "parameter names.*without types") { $cat = "A8-参数无类型(CN类型级联)" }
    # B类: 函数类型冲突
    elseif ($err -match "conflicting types") { $cat = "B-函数类型冲突" }
    # C类: 结构体指针/成员类型错误
    elseif ($err -match "assignment to 'struct.*\*' from incompatible pointer type 'struct") { $cat = "C1-结构体指针类型不兼容赋值" }
    elseif ($err -match "assignment to 'struct") { $cat = "C2-结构体赋值错误" }
    elseif ($err -match "has no member named") { $cat = "C3-结构体无此成员" }
    elseif ($err -match "request for member.*not a structure") { $cat = "C4-非结构体访问成员" }
    elseif ($err -match "request for member") { $cat = "C5-成员访问错误" }
    # D类: 整数赋值类型错误
    elseif ($err -match "assignment to 'long long int' from 'struct") { $cat = "D1-结构体指针赋给整数" }
    elseif ($err -match "assignment to 'long long int' from 'char \*'") { $cat = "D2-字符串指针赋给整数" }
    elseif ($err -match "makes integer from pointer") { $cat = "D3-指针转整数" }
    elseif ($err -match "assignment to 'long long int' from") { $cat = "D4-其他整数赋值错误" }
    # E类: 字符/字符串混淆
    elseif ($err -match "assignment to 'char \*' from 'char'") { $cat = "E1-char*<-char" }
    elseif ($err -match "assignment to 'char' from 'char \*'") { $cat = "E2-char<-char*" }
    elseif ($err -match "invalid type argument of unary '\*'") { $cat = "E3-无效解引用" }
    # F类: 隐式声明级联
    elseif ($err -match "implicit declaration") { $cat = "F-隐式函数声明" }
    elseif ($err -match "assignment to 'char \*' from 'int'|assignment to 'void \*' from 'int'") { $cat = "G-隐式声明级联赋值" }
    # H类: 函数参数错误
    elseif ($err -match "incompatible type for argument") { $cat = "H1-函数参数类型不兼容" }
    elseif ($err -match "passing argument.*incompati") { $cat = "H2-函数参数传递不兼容" }
    elseif ($err -match "too few arguments") { $cat = "H3-函数参数不足" }
    # I类: 表达式语法错误
    elseif ($err -match "expected expression before ';'") { $cat = "I1-空表达式" }
    elseif ($err -match "expected expression before") { $cat = "I2-表达式语法错误" }
    elseif ($err -match "expected '\)' before") { $cat = "I3-括号不匹配" }
    # J类: 变量未声明/重定义
    elseif ($err -match "undeclared|was not declared") { $cat = "J1-变量未声明" }
    elseif ($err -match "redefinition of") { $cat = "J2-重定义" }
    # K类: 其他
    elseif ($err -match "invalid operands to binary") { $cat = "K1-运算符操作数无效" }
    elseif ($err -match "invalid use of void expression") { $cat = "K2-void表达式错误" }
    elseif ($err -match "initializer element is not constant") { $cat = "K3-初始化器非常量" }
    elseif ($err -match "label.*used but not defined") { $cat = "K4-goto标签未定义" }
    elseif ($err -match "returning.*from.*incompati") { $cat = "K5-返回类型不兼容" }
    elseif ($err -match "old-style parameter") { $cat = "K6-旧式参数声明" }
    elseif ($err -match "expected '\{' at end of input") { $cat = "K7-文件末尾缺大括号" }
    elseif ($err -match "dereferencing pointer to incompatible") { $cat = "K8-指针解引用不兼容" }
    elseif ($err -match "initialization of.*from") { $cat = "K9-初始化不匹配" }
    elseif ($err -match "assignment to 'int \*' from incompati") { $cat = "K10-int*赋值不兼容" }
    elseif ($err -match "incompatible types when assigning") { $cat = "K11-赋值类型不兼容" }
    
    if (-not $categories.ContainsKey($cat)) { $categories[$cat] = 0 }
    $categories[$cat]++
}

Write-Output ""
Write-Output "=== 最终精细分类 ==="
$categories.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | ForEach-Object {
    Write-Output "$($_.Value.ToString().PadLeft(5)) | $($_.Key)"
}

Write-Output ""
Write-Output "=== 大类汇总 ==="
$majorCats = @{}
foreach ($e in $allErrors) {
    $err = $e.Error
    $cat = "Z-其他"
    
    if ($err -match "unknown type name|type defaults to 'int' in declaration|expected '=', ',', ';', 'asm' or '__attribute__' before '\{'|expected '=', ',', ';', 'asm' or '__attribute__' before '->'|expected declaration specifiers before|expected '=', ',', ';', 'asm' or '__attribute__' before|expected identifier or '\(' before|parameter names.*without types") { $cat = "A-CN类型名未翻译(级联错误)" }
    elseif ($err -match "conflicting types") { $cat = "B-函数类型冲突" }
    elseif ($err -match "assignment to 'struct|has no member named|request for member") { $cat = "C-结构体指针/成员类型错误" }
    elseif ($err -match "assignment to 'long long int' from|makes integer from pointer") { $cat = "D-整数赋值类型错误" }
    elseif ($err -match "assignment to 'char \*' from 'char'|assignment to 'char' from 'char \*'|invalid type argument of unary") { $cat = "E-字符/字符串类型混淆" }
    elseif ($err -match "implicit declaration") { $cat = "F-隐式函数声明" }
    elseif ($err -match "assignment to 'char \*' from 'int'|assignment to 'void \*' from 'int'") { $cat = "G-隐式声明级联赋值" }
    elseif ($err -match "incompatible type for argument|passing argument.*incompati|too few arguments") { $cat = "H-函数参数错误" }
    elseif ($err -match "expected expression before|expected '\)' before") { $cat = "I-表达式语法错误" }
    elseif ($err -match "undeclared|was not declared|redefinition of") { $cat = "J-变量声明错误" }
    elseif ($err -match "invalid operands to binary|invalid use of void|initializer element is not constant|label.*used but not defined|returning.*from.*incompati|old-style parameter|expected '\{' at end|dereferencing pointer to incompatible|initialization of.*from|assignment to 'int \*' from incompati|incompatible types when assigning") { $cat = "K-其他编译错误" }
    
    if (-not $majorCats.ContainsKey($cat)) { $majorCats[$cat] = 0 }
    $majorCats[$cat]++
}

$majorCats.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | ForEach-Object {
    Write-Output "$($_.Value.ToString().PadLeft(5)) | $($_.Key)"
}

# 统计unknown type name的具体类型
Write-Output ""
Write-Output "=== unknown type name 具体类型统计 ==="
$typeNames = @{}
foreach ($e in $allErrors) {
    if ($e.Error -match "unknown type name '(.+?)'") {
        $tn = $Matches[1]
        if (-not $typeNames.ContainsKey($tn)) { $typeNames[$tn] = 0 }
        $typeNames[$tn]++
    }
}
$typeNames.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | Select-Object -First 20 | ForEach-Object {
    Write-Output "$($_.Value.ToString().PadLeft(5)) | $($_.Key)"
}
