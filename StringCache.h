#pragma once

#include <string>
#include <string_view>
#include <unordered_set>

namespace DdsPerfTest
{
    // This hasher allows an unordered container to work with both std::string and std::string_view,
    // enabling efficient, non-allocating lookups.
    struct StringViewHasher {
        using is_transparent = void; // This enables transparent lookup
        [[nodiscard]] size_t operator()(const char* txt) const {
            return std::hash<std::string_view>{}(txt);
        }
        [[nodiscard]] size_t operator()(std::string_view txt) const {
            return std::hash<std::string_view>{}(txt);
        }
        [[nodiscard]] size_t operator()(const std::string& txt) const {
            return std::hash<std::string>{}(txt);
        }
    };
}
