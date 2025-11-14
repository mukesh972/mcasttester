#include "Miracast.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <json/json.h>

// helper for curl write callback
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    auto realSize = size * nmemb;
    std::string *s = reinterpret_cast<std::string*>(userp);
    s->append(reinterpret_cast<char*>(contents), realSize);
    return realSize;
}

// helper to serialize Json::Value to string
static std::string jsonToString(const Json::Value &v) {
    Json::StreamWriterBuilder w;
    w["indentation"] = ""; // compact
    return Json::writeString(w, v);
}

// helper to parse string into Json::Value
bool parseJson(const std::string &s, Json::Value &out) {
    Json::CharReaderBuilder b;
    std::string errs;
    std::unique_ptr<Json::CharReader> reader(b.newCharReader());
    return reader->parse(s.c_str(), s.c_str() + s.size(), &out, &errs);
}

// normalize controller URL or fallback compile-time default
static std::string normalize_controller_url(const std::string &url) {
    if (!url.empty()) return url;
#ifdef THUNDER_JSONRPC_URL
    return std::string(THUNDER_JSONRPC_URL);
#else
    return std::string("http://127.0.0.1:9998/jsonrpc");
#endif
}

// perform JSON-RPC; params is a Json::Value (object or array)
static Json::Value json_rpc_request(const std::string &controllerUrl, const std::string &method, const Json::Value &params, bool &ok) {
    ok = false;
    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[json_rpc] curl_easy_init failed\n";
        return Json::Value();
    }

    static int idCounter = 1;
    Json::Value payload;
    payload["jsonrpc"] = "2.0";
    payload["id"] = idCounter++;
    payload["method"] = method;
    payload["params"] = params;

    std::string payloadStr = jsonToString(payload);
    std::string responseStr;
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, controllerUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)payloadStr.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 6L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[json_rpc] curl perform error: " << curl_easy_strerror(res) << "\n";
    } else {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode >= 200 && httpCode < 300) {
            Json::Value respJson;
            if (parseJson(responseStr, respJson)) {
                ok = true;
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                return respJson;
            } else {
                std::cerr << "[json_rpc] parse error for response: " << responseStr << "\n";
            }
        } else {
            std::cerr << "[json_rpc] HTTP code: " << httpCode << " body: " << responseStr << "\n";
        }
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return Json::Value();
}

/* MiracastService implementations */

bool activate_service(const std::string &controllerUrl) {
    bool ok = false;
    Json::Value params;
    params["callsign"] = "org.rdk.MiracastService";
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "Controller.activate", params, ok);
    if (!ok) return false;
    std::cout << "[activate_service] " << jsonToString(resp) << "\n";
    return true;
}

bool deactivate_service(const std::string &controllerUrl) {
    bool ok = false;
    Json::Value params;
    params["callsign"] = "org.rdk.MiracastService";
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "Controller.deactivate", params, ok);
    if (!ok) return false;
    std::cout << "[deactivate_service] " << jsonToString(resp) << "\n";
    return true;
}

bool set_enable(const std::string &controllerUrl, bool enabled) {
    bool ok = false;
    Json::Value params;
    params["enabled"] = enabled;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastService.setEnable", params, ok);
    if (!ok) return false;
    std::cout << "[set_enable] " << jsonToString(resp) << "\n";
    return true;
}

bool get_enable(const std::string &controllerUrl, bool &enabled_out) {
    bool ok = false;
    Json::Value params = Json::Value(Json::arrayValue); // no params
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastService.getEnable", params, ok);
    if (!ok) return false;
    std::cout << "[get_enable] " << jsonToString(resp) << "\n";
    try {
        if (resp.isMember("result") && resp["result"].isArray() && resp["result"].size() > 0) {
            const Json::Value &r = resp["result"][0];
            if (r.isMember("enabled")) {
                enabled_out = r["enabled"].asBool();
                return true;
            }
        }
    } catch(...) {}
    return false;
}

bool accept_client_connection(const std::string &controllerUrl, const std::string &requestStatus) {
    bool ok = false;
    Json::Value params;
    params["requestStatus"] = requestStatus;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastService.acceptClientConnection", params, ok);
    if (!ok) return false;
    std::cout << "[accept_client_connection] " << jsonToString(resp) << "\n";
    return true;
}

bool stop_client_connection(const std::string &controllerUrl, const std::string &mac, const std::string &name) {
    bool ok = false;
    Json::Value params;
    params["mac"] = mac;
    params["name"] = name;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastService.stopClientConnection", params, ok);
    if (!ok) return false;
    std::cout << "[stop_client_connection] " << jsonToString(resp) << "\n";
    return true;
}

bool update_player_state(const std::string &controllerUrl, const std::string &mac, const std::string &state, int reason_code, const std::string &reason) {
    bool ok = false;
    Json::Value params;
    params["mac"] = mac;
    params["state"] = state;
    params["reason_code"] = reason_code;
    params["reason"] = reason;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastService.updatePlayerState", params, ok);
    if (!ok) return false;
    std::cout << "[update_player_state] " << jsonToString(resp) << "\n";
    return true;
}

bool set_logging(const std::string &controllerUrl, const std::string &level) {
    bool ok = false;
    Json::Value params;
    params["level"] = level;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastService.setLogging", params, ok);
    if (!ok) return false;
    std::cout << "[set_logging] " << jsonToString(resp) << "\n";
    return true;
}

/* MiracastPlayer implementations */

bool activate_player(const std::string &controllerUrl) {
    bool ok = false;
    Json::Value params;
    params["callsign"] = "org.rdk.MiracastPlayer";
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "Controller.activate", params, ok);
    if (!ok) return false;
    std::cout << "[activate_player] " << jsonToString(resp) << "\n";
    return true;
}

bool deactivate_player(const std::string &controllerUrl) {
    bool ok = false;
    Json::Value params;
    params["callsign"] = "org.rdk.MiracastPlayer";
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "Controller.deactivate", params, ok);
    if (!ok) return false;
    std::cout << "[deactivate_player] " << jsonToString(resp) << "\n";
    return true;
}

bool player_play_request(const std::string &controllerUrl, const DeviceParameters &device_params, const VideoRectangle &rect) {
    bool ok = false;
    Json::Value params;
    Json::Value dev;
    dev["source_dev_ip"] = device_params.source_dev_ip;
    dev["source_dev_mac"] = device_params.source_dev_mac;
    dev["source_dev_name"] = device_params.source_dev_name;
    dev["sink_dev_ip"] = device_params.sink_dev_ip;
    params["device_parameters"] = dev;
    Json::Value rectv;
    rectv["X"] = rect.X;
    rectv["Y"] = rect.Y;
    rectv["W"] = rect.W;
    rectv["H"] = rect.H;
    params["video_rectangle"] = rectv;

    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastPlayer.playRequest", params, ok);
    if (!ok) return false;
    std::cout << "[player_play_request] " << jsonToString(resp) << "\n";
    return true;
}

bool player_stop_request(const std::string &controllerUrl, const std::string &mac, const std::string &name, int reason_code) {
    bool ok = false;
    Json::Value params;
    params["mac"] = mac;
    params["name"] = name;
    params["reason_code"] = reason_code;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastPlayer.stopRequest", params, ok);
    if (!ok) return false;
    std::cout << "[player_stop_request] " << jsonToString(resp) << "\n";
    return true;
}

bool set_player_state(const std::string &controllerUrl, const std::string &state) {
    bool ok = false;
    Json::Value params;
    params["state"] = state;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastPlayer.setPlayerState", params, ok);
    if (!ok) return false;
    std::cout << "[set_player_state] " << jsonToString(resp) << "\n";
    return true;
}

bool set_video_rectangle(const std::string &controllerUrl, const VideoRectangle &rect) {
    bool ok = false;
    Json::Value params;
    params["X"] = rect.X;
    params["Y"] = rect.Y;
    params["W"] = rect.W;
    params["H"] = rect.H;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastPlayer.setVideoRectangle", params, ok);
    if (!ok) return false;
    std::cout << "[set_video_rectangle] " << jsonToString(resp) << "\n";
    return true;
}

bool set_rtsp_wait_timeout(const std::string &controllerUrl, int timeout_ms) {
    bool ok = false;
    Json::Value params;
    params["timeout_ms"] = timeout_ms;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastPlayer.setRTSPWaitTimeout", params, ok);
    if (!ok) return false;
    std::cout << "[set_rtsp_wait_timeout] " << jsonToString(resp) << "\n";
    return true;
}

bool player_set_logging(const std::string &controllerUrl, const std::string &level) {
    bool ok = false;
    Json::Value params;
    params["level"] = level;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastPlayer.setLogging", params, ok);
    if (!ok) return false;
    std::cout << "[player_set_logging] " << jsonToString(resp) << "\n";
    return true;
}

bool set_video_formats(const std::string &controllerUrl, const Json::Value &formats) {
    bool ok = false;
    Json::Value params;
    params["formats"] = formats;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastPlayer.setVideoFormats", params, ok);
    if (!ok) return false;
    std::cout << "[set_video_formats] " << jsonToString(resp) << "\n";
    return true;
}

bool set_audio_formats(const std::string &controllerUrl, const Json::Value &formats) {
    bool ok = false;
    Json::Value params;
    params["formats"] = formats;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.MiracastPlayer.setAudioFormats", params, ok);
    if (!ok) return false;
    std::cout << "[set_audio_formats] " << jsonToString(resp) << "\n";
    return true;
}

bool set_friendly_name(const std::string &controllerUrl, const std::string &friendlyName) {
    bool ok = false;
    Json::Value params;
    params["friendlyName"] = friendlyName;
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.System.1.setFriendlyName", params, ok);
    if (!ok) return false;
    std::cout << "[set_friendly_name] " << jsonToString(resp) << "\n";
    return true;
}

bool get_friendly_name(const std::string &controllerUrl, std::string &friendlyName_out) {
    bool ok = false;
    Json::Value params; 
    auto resp = json_rpc_request(normalize_controller_url(controllerUrl), "org.rdk.System.getFriendlyName", params, ok);
    if (!ok) return false;
    std::cout << "[get_friendly_name] " << jsonToString(resp) << "\n";
    try {
        if (resp.isMember("result")) {
            const Json::Value &r = resp["result"];
            if (r.isMember("friendlyName") && r["friendlyName"].isString()) {
                friendlyName_out = r["friendlyName"].asString();
                return true;
            }
        } else if (resp.isMember("friendlyName") && resp["friendlyName"].isString()) {
            friendlyName_out = resp["friendlyName"].asString();
            return true;
        }
    } catch(...) {}
    return false;
}