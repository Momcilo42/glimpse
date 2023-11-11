#ifndef TAB_HPP_
#define TAB_HPP_

extern "C" {
	#include <ncurses.h> //GUI
    #include <panel.h>
}

// 1s
#define UPDATE_INTERVAL_MS	(1000)

class Tab {
public:
    Tab() {
        int terminal_height;
        int terminal_width;
        getmaxyx(stdscr, terminal_height, terminal_width);
        // Offest height by 1 row for navbar
        tab_window = newwin(terminal_height-navbar_offset, terminal_width, navbar_offset, 0);
        tab_panel = new_panel(tab_window);

        wtimeout(tab_window,(50));
    };
    ~Tab() {
        del_panel(tab_panel);
        delwin(tab_window);
    };

    virtual void update() = 0;

    virtual uint64_t get_pid_at_pos() = 0;

    void proc_up() {
        // If position is already on top, just scroll
        if (proc_table_pos == 0) {
            if (proc_table_top > 0)
                proc_table_top--;
            return;
        }

        proc_table_pos--;
    }

    void proc_down() {
        // If position is already on bottom, just scroll
        if (proc_table_pos == proc_block_size) {
            // Don't scroll if there's less processes than the size of the proc table
            if (proc_block_size >= (proc_table_top + process_vector_size))
                return;
            // Add -1 to account for size(0) counting from 1
            if (proc_table_top < (process_vector_size - proc_block_size - 1))
                proc_table_top++;
            return;
        }

        // -1 to acommodate counting from 0
        if (proc_table_pos < process_vector_size-1)
            proc_table_pos++;
    }

    WINDOW * get_window() {
        return tab_window;
    }
    PANEL * get_panel() {
        return tab_panel;
    }
protected:
    void set_info_block_size(uint8_t new_size) {
        info_block_size = new_size;

        proc_block_start = info_block_size;
        proc_block_size = tab_window->_maxy - proc_block_start;

        if (proc_table_pos > proc_block_size)
            proc_table_pos = proc_block_size;
    }
protected:
    WINDOW * tab_window;
    PANEL * tab_panel;

	// Offset by 1 for navbar
	uint8_t navbar_offset = 1;
    // Top poisition of visible table
    uint32_t proc_table_top = 0;
    // Position in visible table
    uint32_t proc_table_pos = 0;
    // Size of the processes vector in derived classes
    uint32_t process_vector_size = 0;

    // Row on screen at which the info block starts
    uint8_t info_block_start = 0;
    // Row on screen at which the process block starts
    uint32_t proc_block_start = 1;
    // From 0 to size (including size)
    uint8_t info_block_size = 1;
    // From 0 to size (including size)
    uint32_t proc_block_size = 1;
};

#endif // TAB_HPP_