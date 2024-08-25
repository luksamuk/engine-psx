target remote localhost:3333
monitor reset shellhalt
file ./build/engine.elf
load ./build/engine.elf
tbreak main
