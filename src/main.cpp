#include "Miracast.h"
#include "EventListener.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <json/json.h>

static void print_help() {
    std::cout << "Available commands:\n"
              << "  help\n"
              << "  1. activate_service\n"
              << "  2. activate_player\n"
              << "  3. set_enable\n"
              << "  4. accept      # accept last seen client (from events)\n"
              << "  5. quit\n";
}


static std::vector<std::string> split_tokens(const std::string &s) {
    std::istringstream iss(s);
    std::vector<std::string> out;
    std::string token;
    while (iss >> token) out.push_back(token);
    return out;
}

static std::string http_to_ws(const std::string &httpUrl) {
    if (httpUrl.rfind("https://", 0) == 0) {
        return std::string("wss://") + httpUrl.substr(8);
    } else if (httpUrl.rfind("http://", 0) == 0) {
        return std::string("ws://") + httpUrl.substr(7);
    }
    return httpUrl;
}

// search Json::Value recursively for keys "mac" and "name" (return true if found)
static bool find_mac_name(const Json::Value &j, std::string &mac, std::string &name) {
    if (j.isObject()) {
        if (j.isMember("mac") && j.isMember("name") && j["mac"].isString() && j["name"].isString()) {
            mac = j["mac"].asString();
            name = j["name"].asString();
            return true;
        }
        for (const auto &key : j.getMemberNames()) {
            if (find_mac_name(j[key], mac, name)) return true;
        }
    } else if (j.isArray()) {
        for (const auto &el : j) if (find_mac_name(el, mac, name)) return true;
    }
    return false;
}

int main(int argc, char **argv) {
    std::string controllerUrl;
    if (argc > 1) {
        controllerUrl = argv[1];
    } else {
#ifdef THUNDER_JSONRPC_URL
        controllerUrl = std::string(THUNDER_JSONRPC_URL);
#else
        controllerUrl = "http://127.0.0.1:9998/jsonrpc";
#endif
    }
    std::string wsUrl = http_to_ws(controllerUrl);

    std::cout << "Miracast CLI w/ events\nController HTTP JSON-RPC URL: " << controllerUrl << "\n";
    std::cout << "WebSocket event URL (derived): " << wsUrl << "\n";
    print_help();

    // last seen client (from events)
    std::string last_mac;
    std::string last_name;
    std::mutex last_mutex;

    EventListener listener(wsUrl);
    listener.setNotificationCallback([&](const Json::Value &j){
        // Print incoming notification (pretty)
        Json::StreamWriterBuilder w;
        w["indentation"] = "  ";
        std::string pretty = Json::writeString(w, j);
        std::cout << "\n[Event] " << pretty << "\n";

        // Try to detect client connection request (mac/name)
        std::string mac, name;
        if (find_mac_name(j, mac, name)) {
            {
                std::lock_guard<std::mutex> g(last_mutex);
                last_mac = mac;
                last_name = name;
            }
            std::cout << "[Info] Detected client request - mac: " << mac << " name: " << name << "\n";
            std::cout << "[Info] Use 'accept' or 'reject' to respond.\n> " << std::flush;
        } else {
            std::cout << "> " << std::flush;
        }
    });

    if (!listener.start()) {
        std::cerr << "Failed to start event listener. Exiting.\n";
        return 1;
    } else {
        std::cout << "Event listener started.\n";
    }

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        auto tokens = split_tokens(line);
        if (tokens.empty()) continue;
        std::string cmd = tokens[0];

        if (cmd == "help") {
            print_help();
        } else if (cmd == "1" || cmd == "activate_service") {
            if (!activate_service(controllerUrl)) std::cerr << "activate_service failed\n";
        } else if (cmd == "deactivate_service") {
            if (!deactivate_service(controllerUrl)) std::cerr << "deactivate_service failed\n";
        } else if (cmd == "2" || cmd == "activate_player") {
            if (!activate_player(controllerUrl)) std::cerr << "activate_player failed\n";
        } else if (cmd == "deactivate_player") {
            if (!deactivate_player(controllerUrl)) std::cerr << "deactivate_player failed\n";
        } else if (cmd == "3" || cmd == "set_enable") {
             /*if (tokens.size() != 2) { std::cerr << "Usage: set_enable true|false\n"; continue; }
            std::string v = tokens[1]; std::transform(v.begin(), v.end(), v.begin(), ::tolower);*/
            std::string v = "1";
            bool val = (v == "true" || v == "1");
            if (!set_enable(controllerUrl, val)) std::cerr << "set_enable failed\n";
        } else if (cmd == "get_enable") {
            bool enabled;
            if (!get_enable(controllerUrl, enabled)) std::cerr << "get_enable failed\n";
            else std::cout << "enabled = " << (enabled ? "true" : "false") << "\n";
        } else if (cmd == "4" || cmd == "accept") {
            std::lock_guard<std::mutex> g(last_mutex);
            if (last_mac.empty()) { std::cerr << "No client known from events. Wait for onClientConnectionRequest.\n"; }
            else {
                if (!accept_client_connection(controllerUrl, "Accept")) std::cerr << "accept failed\n";
                else std::cout << "Sent Accept for " << last_mac << " / " << last_name << "\n";
            }
        } else if (cmd == "reject") {
            std::lock_guard<std::mutex> g(last_mutex);
            if (last_mac.empty()) { std::cerr << "No client known from events.\n"; }
            else {
                if (!accept_client_connection(controllerUrl, "Reject")) std::cerr << "reject failed\n";
                else std::cout << "Sent Reject for " << last_mac << " / " << last_name << "\n";
            }
        } else if (cmd == "play") {
            if (tokens.size() < 9) { std::cerr << "Usage: play <source_ip> <source_mac> <source_name> <sink_ip> <X> <Y> <W> <H>\n"; continue; }
            DeviceParameters dp; dp.source_dev_ip = tokens[1]; dp.source_dev_mac = tokens[2];
            dp.source_dev_name = tokens[3]; dp.sink_dev_ip = tokens[4];
            VideoRectangle rect;
            try { rect.X = std::stoi(tokens[5]); rect.Y = std::stoi(tokens[6]); rect.W = std::stoi(tokens[7]); rect.H = std::stoi(tokens[8]); }
            catch(...) { std::cerr << "Invalid numbers\n"; continue; }
            if (!player_play_request(controllerUrl, dp, rect)) std::cerr << "playRequest failed\n";
            else std::cout << "playRequest sent\n";
        } else if (cmd == "stop") {
            if (tokens.size() != 4) { std::cerr << "Usage: stop <mac> <name> <reason_code>\n"; continue; }
            if (!player_stop_request(controllerUrl, tokens[1], tokens[2], std::stoi(tokens[3]))) std::cerr << "stopRequest failed\n";
        } else if (cmd == "update") {
            if (tokens.size() < 5) { std::cerr << "Usage: update <mac> <state> <reason_code> <reason...>\n"; continue; }
            std::string mac = tokens[1]; std::string state = tokens[2]; int reason_code = 0;
            try { reason_code = std::stoi(tokens[3]); } catch(...) {}
            std::string reason;
            for (size_t i=4;i<tokens.size();++i) { if (i>4) reason += " "; reason += tokens[i]; }
            if (!update_player_state(controllerUrl, mac, state, reason_code, reason)) std::cerr << "update failed\n";
        } else if (cmd == "5" || cmd == "quit" || cmd == "exit") {
            break;
        } else {
            std::cerr << "Unknown command\n";
        }
    }

    std::cout << "Shutting down event listener...\n";
    listener.stop();
    std::cout << "Exit\n";
    return 0;
}