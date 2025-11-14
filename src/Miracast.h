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

bool parseJson(const std::string &s, Json::Value &out);

// MiracastService methods/events
bool activate_service(const std::string &controllerUrl);
bool deactivate_service(const std::string &controllerUrl);
bool set_enable(const std::string &controllerUrl, bool enabled);
bool get_enable(const std::string &controllerUrl, bool &enabled_out);
bool accept_client_connection(const std::string &controllerUrl, const std::string &requestStatus);
bool stop_client_connection(const std::string &controllerUrl, const std::string &mac, const std::string &name);
bool update_player_state(const std::string &controllerUrl, const std::string &mac, const std::string &state, int reason_code, const std::string &reason);
bool set_logging(const std::string &controllerUrl, const std::string &level);

// MiracastPlayer methods
bool activate_player(const std::string &controllerUrl);
bool deactivate_player(const std::string &controllerUrl);
bool player_play_request(const std::string &controllerUrl, const DeviceParameters &device_params, const VideoRectangle &rect);
bool player_stop_request(const std::string &controllerUrl, const std::string &mac, const std::string &name, int reason_code);
bool set_player_state(const std::string &controllerUrl, const std::string &state);
bool set_video_rectangle(const std::string &controllerUrl, const VideoRectangle &rect);
bool set_rtsp_wait_timeout(const std::string &controllerUrl, int timeout_ms);
bool player_set_logging(const std::string &controllerUrl, const std::string &level);
bool set_video_formats(const std::string &controllerUrl, const Json::Value &formats);
bool set_audio_formats(const std::string &controllerUrl, const Json::Value &formats);

// Function to set/get friendly name
bool set_friendly_name(const std::string &controllerUrl, const std::string &friendlyName);
bool get_friendly_name(const std::string &controllerUrl, std::string &friendlyName_out);
