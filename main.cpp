#include "turing.hpp"
#include "load_turing.hpp"
#include <ncurses.h>
#include <list>
#include <atomic>
#include <chrono>
#include <memory>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std::chrono_literals;
using namespace turing;

int main(int argc, char **argv)
{
    //Initialize NCurses
    initscr();
    start_color();
    use_default_colors();
    noecho();
    curs_set(0);

    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_RED, -1);
    init_pair(3, -1, COLOR_RED);
    init_pair(4, COLOR_YELLOW, -1);
    init_pair(5, COLOR_GREEN, -1);
    init_pair(6, COLOR_MAGENTA, -1);
    init_pair(7, COLOR_BLACK, COLOR_WHITE);

    std::mutex mtx_machines;
    std::vector<std::shared_ptr<machine<std::string, char>>> machines = {};
    size_t selected = 0;
    
    std::mutex mtx_screen;
    std::atomic<bool> quit = false;
    std::atomic<bool> input_mode = false;

    std::string filename = "";
    std::string msg = "";

    std::thread screen_thread([&]{
        while(!quit)
        {
            std::unique_lock l(mtx_screen);

            clear();

            attron(COLOR_PAIR(1));
            for(size_t i = 0; i < 62; ++i)
                    mvprintw(0, i, " ");
            if(input_mode)
            {
                mvprintw(0, 1, "enter: open   esc: cancel");
            }
            else
            {
                mvprintw(0, 1, "r: open   ws: navigate   e: play/pause   x: delete   q: quit");
            }
            attroff(COLOR_PAIR(1));

            if(input_mode)
            {
                attron(COLOR_PAIR(7));
                for(size_t i = 0; i < 62; ++i)
                    mvprintw(1, i, " ");
                mvprintw(1, 0, "File: %s_", filename.c_str());
                attroff(COLOR_PAIR(1));
            }
            else
            {
                attron(COLOR_PAIR(2));
                mvprintw(1, 0, msg.c_str());
                attroff(COLOR_PAIR(2));
            }
            

            {
                size_t i = 0;
                std::unique_lock l(mtx_machines);
                for(const auto& m : machines)
                {
                    size_t y = i*4+2;

                    {
                        std::unique_lock l(m->mtx);

                        if (i==selected) attron(COLOR_PAIR(2));
                        mvprintw(y+0, 1, "#%d", i);
                        attroff(COLOR_PAIR(2));
                        mvprintw(y+1, 1, "Halted:");
                        
                        if (m->get_halted()) attron(COLOR_PAIR(5));
                        else attron(COLOR_PAIR(2));
                        mvprintw(y+1, 9, m->get_halted()?"Yes":"No");
                        if (m->get_halted()) attroff(COLOR_PAIR(5));
                        else attroff(COLOR_PAIR(2));
                        
                        mvprintw(y+1, 13, "State:");

                        attron(COLOR_PAIR(4));
                        mvprintw(y+1, 20, m->get_state().c_str());
                        attroff(COLOR_PAIR(4));

                        mvprintw(y+2, 1, std::string(m->get_tape().data(), m->get_tape().size()).c_str());
                        
                        attron(COLOR_PAIR(6));
                        mvprintw(y+3, 1+m->get_index(), "^");
                        attroff(COLOR_PAIR(6));

                        if (i==selected)
                        {
                            attron(COLOR_PAIR(3));
                            for(size_t i = 0; i < 4; ++ i)
                            {
                                mvprintw(y+i, 0, " ");
                            }
                            attroff(COLOR_PAIR(3));
                        }
                    }

                    ++i;
                }
            }

            refresh();
            std::this_thread::sleep_for(20ms);
        }
    });

    while(!quit)
    {
        switch(getch())
        {
            case 'q': // Quit application
            {
                quit = true;
            }
            break;

            case 'e': // Play/Pause
            {
                if (machines.size() > 0)
                {
                    machines[selected]->set_running(!machines[selected]->get_running());
                }
            }
            break;

            case 'x': // Remove machine
            {
                std::unique_lock l(mtx_machines);
                if (machines.size() > 0)
                {
                    machines.erase(machines.begin()+selected);

                    if (selected > machines.size()-1)
                    {
                        if (machines.size() == 0)
                        {
                            selected = 0;
                        }
                        else
                        {
                            selected = machines.size()-1;
                        }
                    }
                }
            }
            break;

            case 'w': // Navigation (Prev)
            {
                std::unique_lock l(mtx_machines);
                if (selected > 0)
                    --selected;
            }
            break;

            case 's': // Navigation (Next)
            {
                std::unique_lock l(mtx_machines);
                if (selected < machines.size()-1)
                    ++selected;
            }
            break;

            case 'r': // Load new machine
            {
                input_mode = true;
                while(input_mode)
                {
                    int c = getch();
                    switch(c)
                    {
                        case 0x0A: //Enter
                        {
                            std::stringstream buffer;
                            std::ifstream fs(filename);

                            if(fs.fail())
                            {
                                msg = "Error: cannot read file.";
                            }
                            else
                            {
                                msg = "";
                            
                                buffer << fs.rdbuf();
                                const auto m = std::make_shared<machine<std::string, char>>(
                                    import_txt(buffer.str()));

                                std::unique_lock l(mtx_machines);
                                machines.push_back(m);
                            }

                            input_mode = false;
                        }
                        break;

                        case 0x1B: //Escape
                        input_mode = false;
                        break;

                        case 0x7F: //Backspace
                        if (filename.size() > 0)
                            filename.pop_back();
                        break;

                        default:
                        filename += c;
                    }
                }
            }
            break;
        }
    }

    //Stop thread
    screen_thread.join();

    //Stop NCurses
    endwin();

    return EXIT_SUCCESS;
}
