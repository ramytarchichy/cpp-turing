#pragma once

#include <unordered_map>

// Hashing is needed for unordered_map to work
template<class S, class T>
struct std::hash<std::pair<S, T>>
{
    std::size_t operator()(const std::pair<S, T>& k) const
    {
        return ((std::hash<S>()(k.first)
            ^ (std::hash<T>()(k.second) << 1)) >> 1);
    }
};
