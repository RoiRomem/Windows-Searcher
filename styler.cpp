#include <fstream>
#include <map>
#include <variant>

#include "Searcher.h"

using ConfigValue = std::variant<std::byte, std::string>;
std::map<std::string, ConfigValue> parseConfig(const std::string& filename) {
    std::map<std::string, ConfigValue> config;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        auto pos = line.find('=');
        if(pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Trim whitespace
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));

        try {
            int num = std::stoi(value);
            if(num >= 0 && num <= 255) {
                config[key] = std::byte{static_cast<unsigned char>(num)};
            } else {
                config[key] = value;
            }
        } catch (...) {
            config[key] = value;
        }
    }
    return config;
}

void SetValues() {
    const auto styles = parseConfig(CNF_NAME);

    edge_radius = std::get<std::byte>(styles.at("win_edge_radius"));

    font = std::get<std::string>(styles.at("font"));

    // Fixed 3 errors: Add byte conversion for color channels
    bk_color[0] = std::get<std::byte>(styles.at("bk_r"));
    bk_color[1] = std::get<std::byte>(styles.at("bk_g"));
    bk_color[2] = std::get<std::byte>(styles.at("bk_b"));
    bk_color[3] = std::get<std::byte>(styles.at("bk_a"));

    txt_color[0] = std::get<std::byte>(styles.at("txt_r"));
    txt_color[1] = std::get<std::byte>(styles.at("txt_g"));
    txt_color[2] = std::get<std::byte>(styles.at("txt_b"));

    sel_color[0] = std::get<std::byte>(styles.at("sel_r"));
    sel_color[1] = std::get<std::byte>(styles.at("sel_g"));
    sel_color[2] = std::get<std::byte>(styles.at("sel_b"));

    selo_color[0] = std::get<std::byte>(styles.at("selo_r"));
    selo_color[1] = std::get<std::byte>(styles.at("selo_g"));
    selo_color[2] = std::get<std::byte>(styles.at("selo_b"));
}