target remote localhost:3333
symbol-file ./build/sonic.elf
monitor reset shellhalt
load ./build/sonic.elf
directory ./src ./include
tbreak main

