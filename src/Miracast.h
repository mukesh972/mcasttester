#pragma once

#include <string>
#include <json/json.h>

struct DeviceParameters {
    std::string source_dev_ip;
    std::string source_dev_mac;
    std::string source_dev_name;
    std::string sink_dev_ip;
};

struct VideoRectangle {
    int X;
    int Y;
    int W;
    int H;
};

// Miracast RPC wrappers (no simulation): return true on success, false on failure
bool activate_service(const std::string &controllerUrl);
bool deactivate_service(const std::string &controllerUrl);
bool activate_player(const std::string &controllerUrl);
bool deactivate_player(const std::string &controllerUrl);

bool set_enable(const std::string &controllerUrl, bool enabled);
bool get_enable(const std::string &controllerUrl, bool &enabled_out);

bool accept_client_connection(const std::string &controllerUrl, const std::string &requestStatus);
bool stop_client_connection(const std::string &controllerUrl, const std::string &mac, const std::string &name);
bool update_player_state(const std::string &controllerUrl, const std::string &mac, const std::string &state, int reason_code, const std::string &reason);

bool player_play_request(const std::string &controllerUrl, const DeviceParameters &device_params, const VideoRectangle &rect);
bool player_stop_request(const std::string &controllerUrl, const std::string &mac, const std::string &name, int reason_code);