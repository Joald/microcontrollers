# Gietar Hiero 
Guitar Hero/Rock Band/similar games clone made for STM32F411 as part of the Microcontroller Programming course

## Program structure

- `lib` directory contains code that was reused or modified from previous assignments
  - `keyboard.c` scans the keyboard and places the results in a buffer
  - `dma_uart.c` sends debugging messages to the UART
  - `lcd.c` contains all screen drawing primitives (as well as the instructor-provided basic driver)
- Main `gietar-hiero` directory
  - `game.c` contains all game logic concerning spawning/despawning/moving notes
  - `speaker.c` contains a very basic driver for playing monotone sounds
  - `gietar_hiero_main.c` contains the game timer counter and the main loop, which:
    - reads the keyboard buffer and processes the actions
    - instructs the game logic to move notes by the amount of ticks given by the counter

## Compilation 

- build using `make` inside the folder containing `gietar_hiero_main.c`
- Two build modes are supported (but require manual Makefile editing)
  - default mode is no debug
  - switching to debug requires removing `-DNDEBUG` from the `CFLAGS` variable 
    and adding `dma_uart.c` to `LIB_SRC`
  - no debug mode makes all of the functions declared in `dma_uart.h` noops, which 
    optimizes away all of the calling code


## Asset files

All assets are stored in .txt files, in such a way that they can be #included inside an array declaration.
- Images for the board and the notes were created in GIMP, exported to bmp using the default color palette
  used by the LCD. Then the extract_bmp.py script can be used to convert those to a C array contents.
- Note wavelengths are precalculated, more details in the code where they're included.
- The song can be generated in any way that fits the data structure, but an example is in generate_song.py.
