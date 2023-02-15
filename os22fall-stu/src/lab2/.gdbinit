display/x $sstatus
display/z $x6
display/z $x7
set disassemble-next-line on
file vmlinux
target remote :1234
b _start