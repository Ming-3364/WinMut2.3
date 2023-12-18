; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mcpu=generic -mtriple=i686-unknown-unknown | FileCheck %s --check-prefix=X32
; RUN: llc < %s -mcpu=generic -mtriple=x86_64-linux | FileCheck %s --check-prefixes=X64,X64-LINUX
; RUN: llc < %s -mcpu=generic -mtriple=x86_64-win32 | FileCheck %s --check-prefixes=X64,X64-WIN32

declare {i32, i1} @llvm.sadd.with.overflow.i32(i32, i32)
declare {i32, i1} @llvm.uadd.with.overflow.i32(i32, i32)

; The immediate can be encoded in a smaller way if the
; instruction is a sub instead of an add.
define i32 @test1(i32 inreg %a) nounwind {
; X32-LABEL: test1:
; X32:       # %bb.0: # %entry
; X32-NEXT:    subl $-128, %eax
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test1:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    subl $-128, %edi
; X64-LINUX-NEXT:    movl %edi, %eax
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test1:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    subl $-128, %ecx
; X64-WIN32-NEXT:    movl %ecx, %eax
; X64-WIN32-NEXT:    retq
entry:
  %b = add i32 %a, 128
  ret i32 %b
}
define i64 @test2(i64 inreg %a) nounwind {
; X32-LABEL: test2:
; X32:       # %bb.0: # %entry
; X32-NEXT:    addl $-2147483648, %eax # imm = 0x80000000
; X32-NEXT:    adcl $0, %edx
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test2:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    subq $-2147483648, %rdi # imm = 0x80000000
; X64-LINUX-NEXT:    movq %rdi, %rax
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test2:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    subq $-2147483648, %rcx # imm = 0x80000000
; X64-WIN32-NEXT:    movq %rcx, %rax
; X64-WIN32-NEXT:    retq
entry:
  %b = add i64 %a, 2147483648
  ret i64 %b
}
define i64 @test3(i64 inreg %a) nounwind {
; X32-LABEL: test3:
; X32:       # %bb.0: # %entry
; X32-NEXT:    addl $128, %eax
; X32-NEXT:    adcl $0, %edx
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test3:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    subq $-128, %rdi
; X64-LINUX-NEXT:    movq %rdi, %rax
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test3:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    subq $-128, %rcx
; X64-WIN32-NEXT:    movq %rcx, %rax
; X64-WIN32-NEXT:    retq
entry:
  %b = add i64 %a, 128
  ret i64 %b
}

define i1 @test4(i32 %v1, i32 %v2, i32* %X) nounwind {
; X32-LABEL: test4:
; X32:       # %bb.0: # %entry
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    addl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    jo .LBB3_2
; X32-NEXT:  # %bb.1: # %normal
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl $0, (%eax)
; X32-NEXT:  .LBB3_2: # %overflow
; X32-NEXT:    xorl %eax, %eax
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test4:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    addl %esi, %edi
; X64-LINUX-NEXT:    jo .LBB3_2
; X64-LINUX-NEXT:  # %bb.1: # %normal
; X64-LINUX-NEXT:    movl $0, (%rdx)
; X64-LINUX-NEXT:  .LBB3_2: # %overflow
; X64-LINUX-NEXT:    xorl %eax, %eax
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test4:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    addl %edx, %ecx
; X64-WIN32-NEXT:    jo .LBB3_2
; X64-WIN32-NEXT:  # %bb.1: # %normal
; X64-WIN32-NEXT:    movl $0, (%r8)
; X64-WIN32-NEXT:  .LBB3_2: # %overflow
; X64-WIN32-NEXT:    xorl %eax, %eax
; X64-WIN32-NEXT:    retq
entry:
  %t = call {i32, i1} @llvm.sadd.with.overflow.i32(i32 %v1, i32 %v2)
  %sum = extractvalue {i32, i1} %t, 0
  %obit = extractvalue {i32, i1} %t, 1
  br i1 %obit, label %overflow, label %normal

normal:
  store i32 0, i32* %X
  br label %overflow

overflow:
  ret i1 false
}

define i1 @test5(i32 %v1, i32 %v2, i32* %X) nounwind {
; X32-LABEL: test5:
; X32:       # %bb.0: # %entry
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    addl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    jb .LBB4_2
; X32-NEXT:  # %bb.1: # %normal
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl $0, (%eax)
; X32-NEXT:  .LBB4_2: # %carry
; X32-NEXT:    xorl %eax, %eax
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test5:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    addl %esi, %edi
; X64-LINUX-NEXT:    jb .LBB4_2
; X64-LINUX-NEXT:  # %bb.1: # %normal
; X64-LINUX-NEXT:    movl $0, (%rdx)
; X64-LINUX-NEXT:  .LBB4_2: # %carry
; X64-LINUX-NEXT:    xorl %eax, %eax
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test5:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    addl %edx, %ecx
; X64-WIN32-NEXT:    jb .LBB4_2
; X64-WIN32-NEXT:  # %bb.1: # %normal
; X64-WIN32-NEXT:    movl $0, (%r8)
; X64-WIN32-NEXT:  .LBB4_2: # %carry
; X64-WIN32-NEXT:    xorl %eax, %eax
; X64-WIN32-NEXT:    retq
entry:
  %t = call {i32, i1} @llvm.uadd.with.overflow.i32(i32 %v1, i32 %v2)
  %sum = extractvalue {i32, i1} %t, 0
  %obit = extractvalue {i32, i1} %t, 1
  br i1 %obit, label %carry, label %normal

normal:
  store i32 0, i32* %X
  br label %carry

carry:
  ret i1 false
}

define i64 @test6(i64 %A, i32 %B) nounwind {
; X32-LABEL: test6:
; X32:       # %bb.0: # %entry
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X32-NEXT:    addl {{[0-9]+}}(%esp), %edx
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test6:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    # kill: def %esi killed %esi def %rsi
; X64-LINUX-NEXT:    shlq $32, %rsi
; X64-LINUX-NEXT:    leaq (%rsi,%rdi), %rax
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test6:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    # kill: def %edx killed %edx def %rdx
; X64-WIN32-NEXT:    shlq $32, %rdx
; X64-WIN32-NEXT:    leaq (%rdx,%rcx), %rax
; X64-WIN32-NEXT:    retq
entry:
  %tmp12 = zext i32 %B to i64
  %tmp3 = shl i64 %tmp12, 32
  %tmp5 = add i64 %tmp3, %A
  ret i64 %tmp5
}

define {i32, i1} @test7(i32 %v1, i32 %v2) nounwind {
; X32-LABEL: test7:
; X32:       # %bb.0: # %entry
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    addl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    setb %dl
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test7:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    addl %esi, %edi
; X64-LINUX-NEXT:    setb %dl
; X64-LINUX-NEXT:    movl %edi, %eax
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test7:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    addl %edx, %ecx
; X64-WIN32-NEXT:    setb %dl
; X64-WIN32-NEXT:    movl %ecx, %eax
; X64-WIN32-NEXT:    retq
entry:
  %t = call {i32, i1} @llvm.uadd.with.overflow.i32(i32 %v1, i32 %v2)
  ret {i32, i1} %t
}

; PR5443
define {i64, i1} @test8(i64 %left, i64 %right) nounwind {
; X32-LABEL: test8:
; X32:       # %bb.0: # %entry
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X32-NEXT:    addl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    adcl {{[0-9]+}}(%esp), %edx
; X32-NEXT:    setb %cl
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test8:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    addq %rsi, %rdi
; X64-LINUX-NEXT:    setb %dl
; X64-LINUX-NEXT:    movq %rdi, %rax
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test8:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    addq %rdx, %rcx
; X64-WIN32-NEXT:    setb %dl
; X64-WIN32-NEXT:    movq %rcx, %rax
; X64-WIN32-NEXT:    retq
entry:
  %extleft = zext i64 %left to i65
  %extright = zext i64 %right to i65
  %sum = add i65 %extleft, %extright
  %res.0 = trunc i65 %sum to i64
  %overflow = and i65 %sum, -18446744073709551616
  %res.1 = icmp ne i65 %overflow, 0
  %final0 = insertvalue {i64, i1} undef, i64 %res.0, 0
  %final1 = insertvalue {i64, i1} %final0, i1 %res.1, 1
  ret {i64, i1} %final1
}

define i32 @test9(i32 %x, i32 %y) nounwind readnone {
; X32-LABEL: test9:
; X32:       # %bb.0: # %entry
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    xorl %ecx, %ecx
; X32-NEXT:    cmpl $10, {{[0-9]+}}(%esp)
; X32-NEXT:    sete %cl
; X32-NEXT:    subl %ecx, %eax
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test9:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    xorl %eax, %eax
; X64-LINUX-NEXT:    cmpl $10, %edi
; X64-LINUX-NEXT:    sete %al
; X64-LINUX-NEXT:    subl %eax, %esi
; X64-LINUX-NEXT:    movl %esi, %eax
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test9:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    xorl %eax, %eax
; X64-WIN32-NEXT:    cmpl $10, %ecx
; X64-WIN32-NEXT:    sete %al
; X64-WIN32-NEXT:    subl %eax, %edx
; X64-WIN32-NEXT:    movl %edx, %eax
; X64-WIN32-NEXT:    retq
entry:
  %cmp = icmp eq i32 %x, 10
  %sub = sext i1 %cmp to i32
  %cond = add i32 %sub, %y
  ret i32 %cond
}

define i1 @test10(i32 %x) nounwind {
; X32-LABEL: test10:
; X32:       # %bb.0: # %entry
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    incl %eax
; X32-NEXT:    seto %al
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test10:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    incl %edi
; X64-LINUX-NEXT:    seto %al
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test10:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    incl %ecx
; X64-WIN32-NEXT:    seto %al
; X64-WIN32-NEXT:    retq
entry:
  %t = call {i32, i1} @llvm.sadd.with.overflow.i32(i32 %x, i32 1)
  %obit = extractvalue {i32, i1} %t, 1
  ret i1 %obit
}

define void @test11(i32* inreg %a) nounwind {
; X32-LABEL: test11:
; X32:       # %bb.0: # %entry
; X32-NEXT:    subl $-128, (%eax)
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test11:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    subl $-128, (%rdi)
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test11:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    subl $-128, (%rcx)
; X64-WIN32-NEXT:    retq
entry:
  %aa = load i32, i32* %a
  %b = add i32 %aa, 128
  store i32 %b, i32* %a
  ret void
}

define void @test12(i64* inreg %a) nounwind {
; X32-LABEL: test12:
; X32:       # %bb.0: # %entry
; X32-NEXT:    addl $-2147483648, (%eax) # imm = 0x80000000
; X32-NEXT:    adcl $0, 4(%eax)
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test12:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    subq $-2147483648, (%rdi) # imm = 0x80000000
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test12:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    subq $-2147483648, (%rcx) # imm = 0x80000000
; X64-WIN32-NEXT:    retq
entry:
  %aa = load i64, i64* %a
  %b = add i64 %aa, 2147483648
  store i64 %b, i64* %a
  ret void
}

define void @test13(i64* inreg %a) nounwind {
; X32-LABEL: test13:
; X32:       # %bb.0: # %entry
; X32-NEXT:    addl $128, (%eax)
; X32-NEXT:    adcl $0, 4(%eax)
; X32-NEXT:    retl
;
; X64-LINUX-LABEL: test13:
; X64-LINUX:       # %bb.0: # %entry
; X64-LINUX-NEXT:    subq $-128, (%rdi)
; X64-LINUX-NEXT:    retq
;
; X64-WIN32-LABEL: test13:
; X64-WIN32:       # %bb.0: # %entry
; X64-WIN32-NEXT:    subq $-128, (%rcx)
; X64-WIN32-NEXT:    retq
entry:
  %aa = load i64, i64* %a
  %b = add i64 %aa, 128
  store i64 %b, i64* %a
  ret void
}
