#ifndef BANKS_CLIENT_H
#define BANKS_CLIENT_H

#include <string>
#include <optional>

enum class PrivilegeLevel {
    INITIAL,
    INTERMEDIATE,
    FULL,
    __TOTAL
};

struct Client {
    std::string name;
    std::string surname;
    std::optional<std::string> address;
    std::optional<std::string> passport;
    PrivilegeLevel privilege_level = PrivilegeLevel::INITIAL;
};

#endif //BANKS_CLIENT_H
