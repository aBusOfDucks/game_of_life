# game_of_life
Simple Conway's Game of Life implementation using c++ Allegro library (https://github.com/liballeg/allegro5).

Read more here:

https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life

Compile with command:

g++ -pthread game_of_life.cpp $(pkg-config allegro-5 allegro_primitives-5 --libs --cflags) -o game_of_life

When running:
- press space to pause / unpause;
- press 'R' to restart the game;
- press any other key to exit.
