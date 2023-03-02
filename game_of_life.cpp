#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <random>
#include <iostream>

// Size (in cells) of a board.
#define WIDTH 80
#define HEIGHT 60

// Size (in pixels) of a cells.
#define CELL_SIZE 10

// Chance for a cell to spawn in the begining.
// CHANCE = n means a 1/n chance to appear.
#define CHANCE 3

// Speed of drawing.
// SPEED = n means about n redraws per secound.
// Only works for n >= 1.
#define SPEED 10

// Class implementing Conway's Game of Life.
class game_of_life{
private:
	bool cells[WIDTH + 2][HEIGHT + 2];
	bool new_cells[WIDTH + 2][HEIGHT + 2];
	std::random_device dev;
	std::mt19937 rng;
	std::uniform_int_distribution<std::mt19937::result_type> cell_generator;
	std::uniform_int_distribution<std::mt19937::result_type> color_generator;
	
	int cell_rainbow[3];
	int cell_rainbow_current;
	int cell_rainbow_mode;
	
	int background_rainbow[3];
	int background_rainbow_current;
	int background_rainbow_mode;
	
	std::mutex mutex_game_states;
	std::mutex mutex_game_board;
	bool end_of_game = false;
	bool pause_state = false;
	bool restart_state = false;
	bool clear_state = false;
	int theme = 1;
	
	int neighbours(int x, int y)
	{
		int ans = 0;
		for(int i = -1; i <= 1; i++)
			for(int j = -1; j <= 1; j++)
				if((i != 0 || j!= 0) && cells[x + i][y + j])
					ans++;
		return ans;
	}
	
	ALLEGRO_COLOR get_background_color(int theme)
	{
		background_rainbow[background_rainbow_current] += background_rainbow_mode;
		if(background_rainbow[background_rainbow_current] % 250 == 0)
		{
			background_rainbow_current++;
			background_rainbow_current %= 3;
			background_rainbow_mode *= -1;
		}
		
		switch(theme)
		{
			case 1:
			default:
				return al_map_rgb(0, 0, 0);
			case 2:
				return al_map_rgb(255, 255, 255);
			case 3:
				return al_map_rgb(255, 0, 0);
			case 4:
				return al_map_rgb(0, 255, 0);
			case 5:
				return al_map_rgb(0, 0, 255);
			case 6:
				return al_map_rgb(200, 0, 200);
			case 7:
				return al_map_rgb(0, 0, 0);
			case 8:
				return al_map_rgb(background_rainbow[0], background_rainbow[1], background_rainbow[2]);
			case 9:
				return al_map_rgb(background_rainbow[0], background_rainbow[1], background_rainbow[2]);
			case 0:
				return al_map_rgb(color_generator(rng), color_generator(rng), color_generator(rng));
		}
	}
	
	ALLEGRO_COLOR get_cell_color(int theme)
	{
		cell_rainbow[cell_rainbow_current] += cell_rainbow_mode;
		if(cell_rainbow[cell_rainbow_current] % 250 == 0)
		{
			cell_rainbow_current++;
			cell_rainbow_current %= 3;
			cell_rainbow_mode *= -1;
		}
		
		switch(theme)
		{
			case 1:
			default:
				return al_map_rgb(255, 255, 255);
			case 2:
				return al_map_rgb(0, 0, 0);
			case 3:
				return al_map_rgb(0, 0, 255);
			case 4:
				return al_map_rgb(255, 0, 0);
			case 5:
				return al_map_rgb(255, 255, 0);
			case 6:
				return al_map_rgb(0, 255, 0);
			case 7:
				return al_map_rgb(cell_rainbow[0], cell_rainbow[1], cell_rainbow[2]);
			case 8:
				return al_map_rgb(255, 255, 255);
			case 9:
				return al_map_rgb(cell_rainbow[0], cell_rainbow[1], cell_rainbow[2]);
			case 0:
				return al_map_rgb(color_generator(rng), color_generator(rng), color_generator(rng));
		}
	}
	
	void clear_board()
	{
		for(int i = 1; i <= WIDTH; i++)
			for(int j = 1; j <= HEIGHT; j++)
				cells[i][j] = false;
	}
	
public:
	game_of_life(){}
	
	void reset()
	{
		std::unique_lock<std::mutex> lock(mutex_game_board);
		for(int i = 1; i <= WIDTH; i++)
			for(int j = 1; j <= HEIGHT; j++)
				cells[i][j] = (cell_generator(rng) == 1);
	}
	
	// Initiates the board. Fills the array "cells" at random. 
	void init()
	{
		cell_rainbow[0] = 0;
		cell_rainbow[1] = 0;
		cell_rainbow[2] = 250;
		cell_rainbow_current = 1;
		cell_rainbow_mode = 10;
		
		background_rainbow[0] = 250;
		background_rainbow[1] = 250;
		background_rainbow[2] = 0;
		background_rainbow_current = 1;
		background_rainbow_mode = -10;
		
		rng = std::mt19937(dev());
		cell_generator = std::uniform_int_distribution<std::mt19937::result_type>(1, CHANCE);
		color_generator = std::uniform_int_distribution<std::mt19937::result_type>(0, 255);
		
		reset();
	}
	
	// Draws the board.
	void draw()
	{
		ALLEGRO_COLOR background_color;
		ALLEGRO_COLOR cell_color;
		{
			std::unique_lock<std::mutex> lock(mutex_game_states);
			background_color = get_background_color(theme);
			cell_color = get_cell_color(theme);
		}
		
		al_clear_to_color(background_color);
		{
			std::unique_lock<std::mutex> lock(mutex_game_board);
			for(int i = 1; i <= WIDTH; i++)
				for(int j = 1; j <= HEIGHT; j++)
					if(cells[i][j])
						al_draw_filled_rectangle((i - 1) * CELL_SIZE, (j - 1) * CELL_SIZE, i * CELL_SIZE, j * CELL_SIZE, cell_color);
		}	
		
		al_flip_display();
	}
	
	// Calculates one step of Game of Life.
	// Returns false if game has ended.
	// Returns true if game has not ended.
	bool step()
	{
		bool res = false;
		{
			std::unique_lock<std::mutex> lock(mutex_game_states);
			if(end_of_game)
				return false;
			if(pause_state)
				return true;
			if(restart_state)
			{
				res = true;
				restart_state = false;
			}
			if(clear_state)
			{
				clear_board();
				clear_state = false;
			}
		}
		
		if(res)
			reset();
		
		std::unique_lock<std::mutex> lock(mutex_game_board);
		
		for(int i = 1; i <= WIDTH; i++)
		{
			for(int j = 1; j <= HEIGHT; j++)
			{
				int n = neighbours(i,j);
				if(cells[i][j])
					new_cells[i][j] = (n == 2 || n == 3);
				else
					new_cells[i][j] = (n == 3);
			}
		}
		
		for(int i = 1; i <= WIDTH; i++)
			for(int j = 1; j <= HEIGHT; j++)
				cells[i][j] = new_cells[i][j];
				
		return true;
	}
	
	void end()
	{
		std::unique_lock<std::mutex> lock(mutex_game_states);
		end_of_game = true;
	}
	
	void pause()
	{
		std::unique_lock<std::mutex> lock(mutex_game_states);
		pause_state = !pause_state;
	}
	
	void restart()
	{
		std::unique_lock<std::mutex> lock(mutex_game_states);
		restart_state = true;
	}
	
	void clear()
	{
		std::unique_lock<std::mutex> lock(mutex_game_states);
		clear_state = true;
	}
	
	void set_theme(int x)
	{
		std::unique_lock<std::mutex> lock(mutex_game_states);
		theme = x;
	}
};



// Runs the game.
// Makes game steps in loop.
// Ends when game is ended.
void game_loop(game_of_life & game)
{
	ALLEGRO_DISPLAY* disp = al_create_display(WIDTH * CELL_SIZE, HEIGHT * CELL_SIZE);
	al_init_primitives_addon();
	bool run = true;
	long long time = 1000000 / SPEED;
	
	while(run)
	{
		game.draw();
		run = game.step();
		usleep(time);
	}
	
	al_destroy_display(disp);
}

// Checks if given key code belongs to a number.
bool is_a_number(int code)
{
	return code >= ALLEGRO_KEY_0 && code <= ALLEGRO_KEY_9;
}


// Given number key code returns the number.
int get_number(int code)
{
	return code - ALLEGRO_KEY_0;
}

// Handles keyboard.
// Pauses the game when space is pressed.
// Chaneges game theme when '1', '2', ... or '0' is pressed.
// Restarts the game when 'R' is pressed.
// Clears the borad (kills all cells) when 'C' is pressed.
// Ends the game when any other key is pressed.
void keyboard_manager(game_of_life & game)
{

	ALLEGRO_TIMER* timer = al_create_timer(1.0 / 30.0);
	ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
	al_register_event_source(queue, al_get_keyboard_event_source());
	al_register_event_source(queue, al_get_timer_event_source(timer));
	ALLEGRO_EVENT event;
	al_start_timer(timer);
	ALLEGRO_KEYBOARD_STATE state;
	bool run = true;
	while(run)
	{
		al_wait_for_event(queue, &event);
		switch(event.type)
		{
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
				game.end();
				run = false;
				break;
				
			case ALLEGRO_EVENT_KEY_DOWN:
				al_get_keyboard_state(&state);
				switch(event.keyboard.keycode)
				{
					case ALLEGRO_KEY_R:
						game.restart();
						break;
						
					case ALLEGRO_KEY_SPACE:
						game.pause();
						break;
						
					case ALLEGRO_KEY_C:
						game.clear();
						break;
						
					default:
						if(is_a_number(event.keyboard.keycode))
							game.set_theme(get_number(event.keyboard.keycode));
						else
						{
							game.end();
							run = false;
						}
						break;
				}
				break;
		}
	}
	al_destroy_timer(timer);
	al_destroy_event_queue(queue);
}

// Initiates everything needed.
void init(game_of_life & game)
{
	al_init();
	al_install_keyboard();
	game.init();
}

int main()
{
	game_of_life game;
	init(game);
	
	std::thread loop([&game]{game_loop(game);});
	std::thread keyboard([&game]{keyboard_manager(game);});
	
	keyboard.join();
	loop.join();
	
	return 0;
}
