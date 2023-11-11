#ifndef NAVBAR_HPP_
#define NAVBAR_HPP_

#include <iostream>
#include <vector>

extern "C" {
	#include <ncurses.h> //GUI
}

class Navbar {
public:
    Navbar();
    ~Navbar() {};

    int pos = 0;
    void move_right();
    void move_left();

    std::vector<std::string> get_tabs() {
        return tabs;
    }
    std::vector<int> get_tab_offsets() {
        return tab_offsets;
    }
    WINDOW * get_navbar() {
        return navbar;
    }
private:
    void select_tab(int p);
    void deselect_tab(int p);

    std::vector<std::string> tabs;
    std::vector<int> tab_offsets;
    WINDOW *navbar;
};

#endif // NAVBAR_HPP_