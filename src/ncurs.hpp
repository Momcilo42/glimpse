#ifndef NCURS_HPP_
#define NCURS_HPP_

#include "navbar.hpp"
#include "tabs/tab.hpp"

void ncurses_init();
void ncurses_check_keyboard(Navbar *navbar, Tab *current_tab);
void ncurses_fini();

#endif // NCURS_HPP_