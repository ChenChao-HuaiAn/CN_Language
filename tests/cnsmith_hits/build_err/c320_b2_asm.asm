; 由 v2 代码生成（P6）生成
.code
cn_main PROC
    push rbp
    mov rbp, rsp
    sub rsp, 240
    ; op=42
    ; op=1
    mov rax, 0
    mov [rbp-16], rax
    ; op=41
    mov rax, [rbp-16]
    mov [rbp-208], rax
    ; op=43
    lea rax, [rbp-208]
    mov [rbp-24], rax
    ; op=43
    lea rax, [rbp-208]
    mov [rbp-32], rax
    ; op=46
    mov rax, [rbp-32]
    test rax, rax
    jne npok_21
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_21:
    mov [rbp-40], rax
    ; op=1
    mov rax, 1
    mov [rbp-48], rax
    ; op=45
    mov rax, [rbp-40]
    test rax, rax
    jne npok_35
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_35:
    mov rcx, [rbp-48]
    mov dword ptr [rax], ecx
    ; op=43
    lea rax, [rbp-208]
    mov [rbp-56], rax
    ; op=46
    mov rax, [rbp-56]
    test rax, rax
    jne npok_50
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_50:
    add rax, 4
    mov [rbp-64], rax
    ; op=1
    mov rax, 0
    mov [rbp-72], rax
    ; op=45
    mov rax, [rbp-64]
    test rax, rax
    jne npok_65
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_65:
    mov rcx, [rbp-72]
    mov dword ptr [rax], ecx
    ; op=42
    ; op=42
    ; op=1
    mov rax, 0
    mov [rbp-80], rax
    ; op=41
    mov rax, [rbp-80]
    mov [rbp-216], rax
    ; op=41
    mov rax, [rbp-80]
    mov [rbp-224], rax
    ; op=43
    lea rax, [rbp-224]
    mov [rbp-88], rax
    ; op=43
    lea rax, [rbp-224]
    mov [rbp-96], rax
    ; op=46
    mov rax, [rbp-96]
    test rax, rax
    jne npok_94
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_94:
    mov [rbp-104], rax
    ; op=1
    mov rax, 2
    mov [rbp-112], rax
    ; op=45
    mov rax, [rbp-104]
    test rax, rax
    jne npok_108
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_108:
    mov rcx, [rbp-112]
    mov dword ptr [rax], ecx
    ; op=43
    lea rax, [rbp-224]
    mov [rbp-120], rax
    ; op=46
    mov rax, [rbp-120]
    test rax, rax
    jne npok_123
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_123:
    add rax, 4
    mov [rbp-128], rax
    ; op=43
    lea rax, [rbp-208]
    mov [rbp-136], rax
    ; op=45
    mov rax, [rbp-128]
    test rax, rax
    jne npok_138
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_138:
    mov rcx, [rbp-136]
    mov [rax], rcx
    ; op=40
    mov rax, [rbp-224]
    mov [rbp-144], rax
    ; op=43
    lea rax, [rbp-224]
    mov [rbp-152], rax
    ; op=46
    mov rax, [rbp-152]
    test rax, rax
    jne npok_156
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_156:
    add rax, 4
    mov [rbp-160], rax
    ; op=46
    mov rax, [rbp-160]
    test rax, rax
    jne npok_168
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_168:
    mov [rbp-168], rax
    ; op=44
    mov rax, [rbp-168]
    test rax, rax
    jne npok_179
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_179:
    movsxd rax, dword ptr [rax]
    mov [rbp-176], rax
    ; op=63
    sub rsp, 32
    mov rcx, [rbp-176]
    call __cn_print_int
    add rsp, 32
    ; op=1
    mov rax, 0
    mov [rbp-184], rax
    ; op=43
    lea rax, [rbp-224]
    mov [rbp-192], rax
    ; op=46
    mov rax, [rbp-192]
    test rax, rax
    jne npok_202
    sub rsp, 32
    mov rcx, 3
    call __cn_runtime_error
    add rsp, 32
    ret
npok_202:
    add rax, 4
    mov [rbp-200], rax
    ; op=53
    mov rax, [rbp-184]
    ; op=50
    jmp bb0_1
    ; op=50
    jmp bb0_1
    ; op=49
bb0_1:
    mov rsp, rbp
    pop rbp
    ret
cn_main ENDP

EXTERN __cn_runtime_error:PROC
EXTERN __cn_print_int:PROC
END
