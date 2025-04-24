target remote localhost:3333
monitor reset shellhalt
file ./build/sonic.elf
load ./build/sonic.elf
tbreak main
