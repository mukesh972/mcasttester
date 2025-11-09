#pragma once
#include <json/json.h>
#include <iostream>
#include <string>

inline void handleClientConnectionError(const Json::Value& j) {
    std::cerr << "[MiracastService][Event] onClientConnectionError:\n";
    if (j.isMember("params")) {
        const Json::Value& params = j["params"];
        std::cerr << "  mac: " << params.get("mac", "").asString() << "\n"
                  << "  name: " << params.get("name", "").asString() << "\n"
                  << "  error: " << params.get("error", "").asString() << "\n";
    } else {
        std::cerr << "  [Missing params]\n" << j.toStyledString() << std::endl;
    }
}

inline void handleClientConnectionRequest(const Json::Value& j) {
    std::cout << "[MiracastService][Event] onClientConnectionRequest:\n";
    if (j.isMember("params")) {
        const Json::Value& params = j["params"];
        std::cout << "  mac: " << params.get("mac", "").asString() << "\n"
                  << "  name: " << params.get("name", "").asString() << "\n"
                  << "  [Info] Use 'accept' or 'reject' to respond.\n";
    } else {
        std::cerr << "  [Missing params]\n" << j.toStyledString() << std::endl;
    }
}

inline void handleLaunchRequest(const Json::Value& j) {
    std::cout << "[MiracastService][Event] onLaunchRequest:\n";
    if (j.isMember("params")) {
        const Json::Value& params = j["params"];
        std::cout << "  name: " << params.get("name", "").asString() << "\n"
                  << "  mac: " << params.get("mac", "").asString() << "\n";
        // Other launch parameters as available
    } else {
        std::cerr << "  [Missing params]\n" << j.toStyledString() << std::endl;
    }
}

inline void dispatchMiracastServiceEvent(const Json::Value& j) {
    if (!j.isObject() || !j.isMember("method"))
        return;
    const std::string& method = j["method"].asString();
    try {
        if (method == "org.rdk.MiracastService.onClientConnectionError")
            handleClientConnectionError(j);
        else if (method == "org.rdk.MiracastService.onClientConnectionRequest")
            handleClientConnectionRequest(j);
        else if (method == "org.rdk.MiracastService.onLaunchRequest")
            handleLaunchRequest(j);
    } catch (const std::exception& ex) {
        std::cerr << "[MiracastService][Exception] " << ex.what() << "\nEvent:\n" << j.toStyledString() << std::endl;
    }
}
