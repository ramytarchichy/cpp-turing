#include "load_turing.hpp"
#include "hash_pair.hpp"
#include <sstream>
#include <iterator>
#include <unordered_map>

using namespace turing;

// Create turing machine from simple text
machine<std::string, char> turing::import_txt(const std::string& text)
{
    // Split string into tokens
    std::istringstream iss(text);
    std::vector<std::string> tokens {
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
    };

    // Read tokens
    auto it = tokens.begin();

    const std::string tape = *(it++);
    const size_t index = std::stoll(*(it++));

    // Create map for machine instructions
    std::unordered_map<
        std::pair<std::string, char>,
        std::optional<std::tuple<std::string, char, movement>>
    > map = {};
    while(it != tokens.end())
    {
        const std::string prev_state = *(it++);
        const char prev_value = (*(it++))[0];
        const char new_value = (*(it++))[0];
        const movement mvt = *(it++)=="L"?movement::left:movement::right;
        const std::string new_state = *(it++);

        map[std::make_pair(prev_state, prev_value)] =
            std::make_optional(std::make_tuple(new_state, new_value, mvt));
    }
    
    // Create and return turing machine
    return machine<std::string, char>(
        std::vector<char>(
            tape.begin(),
            tape.end()
        ),
        index,
        "0",
        map
    );
}
