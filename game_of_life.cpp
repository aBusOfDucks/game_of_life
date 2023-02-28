#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <random>

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

// Default colors of cells and background.
#define CELL_COLOR al_map_rgb(255, 255, 255)
#define BACKGROUND_COLOR al_map_rgb(0, 0, 0)

// Class implementing Conway's Game of Life.
class game_of_life{
private:
	bool cells[WIDTH + 7][HEIGHT + 7];
	bool new_cells[WIDTH + 7][HEIGHT + 7];
	
	int neighbours(int x, int y)
	{
		int ans = 0;
		for(int i = -1; i <= 1; i++)
			for(int j = -1; j <= 1; j++)
				if((i != 0 || j!= 0) && cells[x + i][y + j])
					ans++;
		return ans;
	}
	
public:
	game_of_life(){}
	
	// Initiates the board. Fills the array "cells" at random. 
	void init()
	{
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<std::mt19937::result_type> dist(1, CHANCE);
		
		for(int i = 1; i <= WIDTH; i++)
			for(int j = 1; j <= HEIGHT; j++)
				cells[i][j] = (dist(rng) == 1);
	}
	
	// Draws the board.
	void draw()
	{
		al_clear_to_color(BACKGROUND_COLOR);
		for(int i = 1; i <= WIDTH; i++)
			for(int j = 1; j <= HEIGHT; j++)
				if(cells[i][j])
					al_draw_filled_rectangle((i - 1) * CELL_SIZE, (j - 1) * CELL_SIZE, i * CELL_SIZE, j * CELL_SIZE, CELL_COLOR);
		al_flip_display();
	}
	
	// Calculates one step of Game of Life.
	void step()
	{
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
	}
};

// Class handling concurrent usage of game states.
class concurrent_manager{
private:
	std::mutex mut;
	bool end_of_game = false;
	bool pause = false;
	bool restart = false;
	std::condition_variable cv_pause;
	
public:
	void end_game()
	{
		std::unique_lock<std::mutex> lock(mut);
		end_of_game = true;
		cv_pause.notify_all();
	}
	
	void change_pause()
	{
		std::unique_lock<std::mutex> lock(mut);
		pause = !pause;
		cv_pause.notify_all();
	}
	
	bool get_pause_state()
	{
		std::unique_lock<std::mutex> lock(mut);
		return pause;
	}
	
	bool get_restart_state()
	{
		std::unique_lock<std::mutex> lock(mut);
		return restart;
	}
	
	bool get_end_of_game_state()
	{
		std::unique_lock<std::mutex> lock(mut);
		return end_of_game;
	}
	
	void wait_on_pause()
	{
		std::unique_lock<std::mutex> lock(mut);
		cv_pause.wait(lock, [this]{return (!pause || end_of_game);});
	}
	
	void restart_do()
	{
		std::unique_lock<std::mutex> lock(mut);
		restart = true;
	}
	
	void restart_done()
	{
		std::unique_lock<std::mutex> lock(mut);
		restart = false;
	}
};

// Runs the game.
// Stops when concurrent.pause == true.
// Restarts when concurrent.restart == true.
// Ends when concurrent.end_of_game == true.
void game_loop(concurrent_manager & concurrent, game_of_life & game)
{
	ALLEGRO_DISPLAY* disp = al_create_display(WIDTH * CELL_SIZE, HEIGHT * CELL_SIZE);
	al_init_primitives_addon();
	bool run = true;
	long long time = 1000000 / SPEED;
	
	while(run)
	{
		if(concurrent.get_pause_state())
			concurrent.wait_on_pause();
			
		if(concurrent.get_end_of_game_state())
			run = false;
		
		
		if(run)
		{
			if(concurrent.get_restart_state())
			{
				game.init();
				concurrent.restart_done();
			}
			game.draw();
			game.step();
			usleep(time);
		}
	}
	
	al_destroy_display(disp);
}

// Handles keyboard.
// Changes concurrent.pause on when space is pressed.
// Sets concurrent.restart to true when 'R' is pressed.
// Sets concurrent.end_of_game to false when any other key is pressed.
void keyboard_manager(concurrent_manager & concurrent)
{

	ALLEGRO_TIMER* timer = al_create_timer(1.0 / 30.0);
	ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
	al_register_event_source(queue, al_get_keyboard_event_source());
	al_register_event_source(queue, al_get_timer_event_source(timer));
	ALLEGRO_EVENT event;
	al_start_timer(timer);
	ALLEGRO_KEYBOARD_STATE state;
	bool run = true;
	bool space = false;
	
	while(run)
	{
		al_wait_for_event(queue, &event);
		switch(event.type)
		{
			case ALLEGRO_EVENT_KEY_DOWN:
			
				al_get_keyboard_state(&state);
				space = al_key_down(&state, ALLEGRO_KEY_SPACE);
				switch(event.keyboard.keycode)
				{
					case ALLEGRO_KEY_R:
						concurrent.restart_do();
						break;
					case ALLEGRO_KEY_SPACE:
						concurrent.change_pause();
						break;
					default:
						concurrent.end_game();
						run = false;
						break;
						
					
				}
				break;
			
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
			
				concurrent.end_game();
				run = false;
				
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
	concurrent_manager concurrent;
	init(game);
	
	std::thread loop([&concurrent, &game]{game_loop(concurrent, game);});
	std::thread keyboard([&concurrent]{keyboard_manager(concurrent);});
	
	keyboard.join();
	loop.join();
	
	return 0;
}
