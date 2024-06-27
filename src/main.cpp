#include "util.hpp"
#include "ncurs.hpp"
#include "proc.hpp"

#include "navbar.hpp"
#include "tabs/overview.hpp"
#include "tabs/cpu.hpp"
#include "tabs/gpu.hpp"
#include "tabs/mem.hpp"
#include "tabs/disk.hpp"
#include "tabs/net.hpp"

extern bool quit;

inline void hide_all_panels(std::vector<Tab *> tabs) {
	for (Tab * t : tabs)
		hide_panel(t->get_panel());
}

void select_tab_panel(Tab * tab, std::vector<Tab *> tabs) {
	if (!panel_hidden(tab->get_panel()))
		return;

	hide_all_panels(tabs);
	show_panel(tab->get_panel());
	// Update the stacking order of panels
	update_panels();
}

int main(int argc, char *argv[]) {
	setup_signal_handler();
	check_root();
	read_args(argc, argv);

	ncurses_init();

	std::vector<struct process> processes;
	find_processes(&processes);

	Navbar nav;

	Overview overview_tab;
	CPU cpu_tab;
	GPU gpu_tab;
	MEM mem_tab;
	DISK disk_tab;
	NET net_tab;

	std::vector<Tab *> tabs;
	tabs.push_back(&overview_tab);
	tabs.push_back(&cpu_tab);
	tabs.push_back(&gpu_tab);
	tabs.push_back(&mem_tab);
	tabs.push_back(&disk_tab);
	tabs.push_back(&net_tab);
	hide_all_panels(tabs);

	Tab *selected_tab = nullptr;
	while (!quit) {
		selected_tab = tabs.at(nav.pos);
		select_tab_panel(selected_tab, tabs);

		wrefresh(selected_tab->get_window());

		ncurses_check_keyboard(&nav, selected_tab);

		selected_tab->update();
	}

	ncurses_fini();

	std::cout << "Quitting properly" << std::endl;

	return 0;
}
