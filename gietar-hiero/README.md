# Gietar Hiero 
Guitar Hero/Rock Band/similar games clone made for STM32F411

## Program structure

- `lib` directory
  - `keyboard.c` scans the keyboard and places the results in a buffer
  - `dma_uart.c` sends debugging messages to the UART
  - `lcd.c` contains all screen drawing primitives (as well as the instructor-provided basic driver)
- `gietar-hiero` directory
  - `game.c` contains all game logic concerning spawning/despawning/moving notes
  - `speaker.c` contains a very basic driver for playing monotone sounds
  - `gietar_hiero_main.c` contains the game timer counter and the main loop, which:
    - reads the keyboard buffer and processes the actions
    - instructs the game logic to move notes by the amount of ticks given by the counter

## Compilation 

- build using `make` inside the folder containing `gietar_hiero_main.c`
- Two build modes are supported (but require manual Makefile editing)
  - default mode is debug
  - switching to no debug requires adding `-DNDEBUG` to the `CFLAGS` variable and removing `dma_uart.c` from `LIB_SRC`
  - no debug mode makes all of the functions declared in `dma_uart.h` noops
