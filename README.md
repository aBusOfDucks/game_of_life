# game_of_life
Simple Conway's Game of Life implementation using c++ Allegro library (https://github.com/liballeg/allegro5).

Read more here:

https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life

Compile with command:

g++ -pthread game_of_life.cpp $(pkg-config allegro-5 allegro_primitives-5 --libs --cflags) -o game_of_life

When running:
- press space to pause / unpause;
- press 'R' to restart the game;
- press 'C' to clear the board (kill all cells) without ending the game;
- press '1' - '0' to change theme of the game (colors of background and cells). Predefined themes (cells on background):
  1 - white on black;
  2 - black on white;
  3 - blue on red;
  4 - red on green;
  5 - yellow on blue;
  6 - green on purple;
  7 - rainbow on black;
  8 - white on rainbow;
  9 - rainbow on rainbow;
  0 - random on random (changing rapidly);
- press any other key to exit.
