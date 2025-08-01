define restart-all
  symbol-file ./build/sonic.elf
  monitor reset shellhalt
  load ./build/sonic.elf
  directory ./src ./include
end

target remote localhost:3333
restart-all
tbreak main

