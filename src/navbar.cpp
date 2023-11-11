#include "navbar.hpp"

Navbar::Navbar() {
    int terminal_width = getmaxx(stdscr);
	navbar = subwin(stdscr, 1, terminal_width, 0, 0);

	tabs.push_back("Overview");
	tabs.push_back("CPU");
	tabs.push_back("GPU");
	tabs.push_back("MEM");
	tabs.push_back("DISK");
	tabs.push_back("NET");

	int offset = 0;
	for (int i = 0; i < (int) tabs.size(); i++) {
		mvwprintw(navbar, 0, offset, " %s ", tabs[i].c_str());
		tab_offsets.push_back(offset);
		offset += tabs[i].size() + 2;
	}

	// Set first tab to be "selected"
	wattron(navbar, A_REVERSE);
	mvwprintw(navbar, 0, tab_offsets[0], " %s ", tabs[0].c_str());

	// Refreshes the screen so print can be seen
	refresh();
	// Refresh navbar window
	wrefresh(navbar);
}

void Navbar::select_tab(int p) {
	wattron(navbar, A_REVERSE);
	mvwprintw(navbar, 0, tab_offsets[p], " %s ", tabs[p].c_str());
}

void Navbar::deselect_tab(int p) {
	wattroff(navbar, A_REVERSE);
	mvwprintw(navbar, 0, tab_offsets[p], " %s ", tabs[p].c_str());
}

void Navbar::move_right() {
	deselect_tab(pos);
	pos = (pos + 1) % tabs.size();
	select_tab(pos);
}

void Navbar::move_left() {
	deselect_tab(pos);
	if (--pos < 0)
		pos = tabs.size() - 1;
	select_tab(pos);
}