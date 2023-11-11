#include "ncurs.hpp"

#include <iostream>
#include <vector>
#include <signal.h>

extern "C" {
	#include <ncurses.h> //GUI
}

extern bool quit;
extern bool lock;

void ncurses_init() {
	// init screen and sets up screen
	initscr();
	// Don't print anything on keypress
	noecho();
	// Hide cursor
	curs_set(0);
	// Wait a UPDATE_INTERVAL_MS ms for key press
	timeout(UPDATE_INTERVAL_MS);
	// Enable arrow keys
	keypad(stdscr, TRUE);
	// Allow scrolling
	scrollok(stdscr, TRUE);
}

void ncurses_check_keyboard(Navbar *navbar, Tab *current_tab) {
    std::vector<std::string> tabs = navbar->get_tabs();
    std::vector<int> tab_offsets = navbar->get_tab_offsets();
	// Has a timeout UPDATE_INTERVAL_MS
	int character = wgetch(current_tab->get_window());
	switch (character)
	{
		case 'q':
			quit = true;
			break;
		case 'l':
			lock = !lock;
			break;
		case 'k':
			kill (current_tab->get_pid_at_pos(), SIGKILL);
			current_tab->proc_up();
			wrefresh(current_tab->get_window());
			break;
		case 'i':
			kill (current_tab->get_pid_at_pos(), SIGINT);
			current_tab->proc_up();
			wrefresh(current_tab->get_window());
			break;
		case '2':
		case KEY_RIGHT:
			navbar->move_right();
			break;
		case '1':
		case KEY_LEFT:
			navbar->move_left();
			break;
		case 'w':
		case KEY_UP:
			current_tab->proc_up();
			break;
		case 's':
		case KEY_DOWN:
			current_tab->proc_down();
			break;
		default:
			break;
	}
	// Refresh navbar window
	wrefresh(navbar->get_navbar());
}

void ncurses_fini() {
	// Clear the screen
	clear();
	// deallocates memory and ends ncurses
	endwin();
}