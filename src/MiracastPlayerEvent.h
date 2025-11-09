#pragma once
#include <json/json.h>
#include <iostream>
#include <string>

inline void handlePlayerStateChange(const Json::Value& j) {
    std::cout << "[MiracastPlayer][Event] onStateChange:\n";
    if (j.isMember("params")) {
        const Json::Value& params = j["params"];
        std::cout << "  name: " << params.get("name", "").asString() << "\n"
                  << "  mac: " << params.get("mac", "").asString() << "\n"
                  << "  state: " << params.get("state", "").asString() << "\n"
                  << "  reason_code: " << params.get("reason_code", "").asString() << "\n"
                  << "  reason: " << params.get("reason", "").asString() << "\n";
    } else {
        std::cerr << "  [Missing params]\n" << j.toStyledString() << std::endl;
    }
}

inline void dispatchMiracastPlayerEvent(const Json::Value& j) {
    if (!j.isObject() || !j.isMember("method")) return;
    const std::string& method = j["method"].asString();
    if (method == "org.rdk.MiracastPlayer.onStateChange") {
        try {
            handlePlayerStateChange(j);
        } catch (const std::exception& ex) {
            std::cerr << "[MiracastPlayer][Exception]: " << ex.what()
                      << "\nEvent Content: " << j.toStyledString() << std::endl;
        }
    }
}
