
.section .text,"ax",@progbits
_start:
  mov $4, %eax
  mov $1, %ebx
  mov $str, %ecx
  mov $13, %edx
  int $0x80

  mov $1, %eax
  mov $0, %ebx
  int $0x80

str:
 .ascii "Hello RISC-V\n"
