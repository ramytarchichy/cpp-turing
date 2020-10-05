#pragma once

#include "hash_pair.hpp"
#include <tuple>
#include <mutex>
#include <vector>
#include <thread>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <functional>
#include <condition_variable>

namespace turing
{
    enum class movement
    {
        left,
        right
    };

    template <class S, class T>
    class machine
    {
    public:
        machine(
            const std::vector<T>& tape,
            size_t index,
            const S& state,
            const std::function<std::optional<std::tuple<S, T, movement>>(
                const S&, const T&)>& transition
        ):
            tape(tape),
            index(index),
            state(state),
            transition(transition)
        {
            
        }

        machine(
            const std::vector<T>& tape,
            size_t index,
            const S& state,
            const std::unordered_map<std::pair<S, T>,
                std::optional<std::tuple<S, T, movement>>>& map
        ):
            tape(tape),
            index(index),
            state(state),
            transition(
                [map](const S& state, const T& value)->
                std::optional<std::tuple<S, T, movement>>
                {
                    try
                    {
                        return map.at(std::make_pair(state, value));
                    }
                    catch (const std::out_of_range& e)
                    {
                        return {};
                    }
                }
            )
        {
            
        }

        machine(const machine<S, T>& m):
            tape(m.tape),
            index(m.index),
            state(m.state),
            halted(m.halted),
            transition(m.transition),
            running(m.running.load()),
            quitting(m.quitting.load())
        {
            
        }

        ~machine()
        {
            if (t.joinable())
            {
                quitting=true;
                {
                    std::unique_lock l(mtx_cv);
                    cv.notify_all();
                }
                t.join();
            }
        }

        void step()
        {
            std::unique_lock l(mtx);
            const auto t = transition(state, tape[index]);

            //If it's null, the rule doesn't exist and the machine should halt
            if (t == std::nullopt)
            {
                halted = true;
                return;
            }

            //Update turing machine state
            state = std::get<0>(*t);
            tape[index] = std::get<1>(*t);
            
            //Move tape
            switch (std::get<2>(*t))
            {
            case movement::left:
                {
                    if (index == 0) halted = true;
                    else --index;
                }
                break;

            case movement::right:
                {
                    if (index >= tape.size()-1) halted = true;
                    else ++index;
                }
                break;
            }
        }

        bool get_halted() const
        {
            return halted;
        }

        size_t get_index() const
        {
            return index;
        }
        
        const S& get_state() const
        {
            return state;
        }

        const std::vector<T>& get_tape() const
        {
            return tape;
        }

        bool get_running() const
        {
            return running;
        }

        void set_running(bool value)
        {
            running = value;
            if (value)
            {
                std::unique_lock l(mtx_cv);
                cv.notify_all();
            }
        }

        std::mutex mtx;
        
    private:
        std::thread t = std::thread([&]{
            while(!quitting)
            {
                if(!running || halted)
                {
                    std::unique_lock l(mtx_cv);
                    cv.wait(l);
                }

                step();
            }
        });;
        
        std::atomic<bool> quitting = false;
        std::atomic<bool> running = false;

        std::condition_variable cv;
        std::mutex mtx_cv;

        S state;
        bool halted = false;
        std::vector<T> tape;
        size_t index;
        const std::function<std::optional<std::tuple<S, T, movement>>(
            const S&, const T&)> transition;
    };
}
