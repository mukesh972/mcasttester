#pragma once

#include <functional>
#include <string>
#include <json/json.h>

class EventListener {
public:
    // controllerWsUrl - ws:// or wss:// URL to subscribe to (e.g. ws://127.0.0.1:9998/jsonrpc)
    explicit EventListener(const std::string &controllerWsUrl);
    ~EventListener();

    // start WebSocket client in background thread
    bool start();

    // stop and join
    void stop();

    // callback called on any JSON notification from server (Json::Value)
    void setNotificationCallback(std::function<void(const Json::Value&)> cb);

private:
    struct Impl;
    Impl* pimpl;
};