#include "EventListener.h"
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <thread>
#include <atomic>
#include <iostream>

using Json::Value;
typedef websocketpp::client<websocketpp::config::asio_client> ws_client;

struct EventListener::Impl {
    std::string uri;
    ws_client client;
    websocketpp::connection_hdl hdl;
    std::thread runner;
    std::atomic<bool> running;
    std::function<void(const Value&)> callback;

    Impl(const std::string &u): uri(u), running(false) {
        client.clear_access_channels(websocketpp::log::alevel::all);
        client.clear_error_channels(websocketpp::log::elevel::all);
        client.init_asio();
    }

    ~Impl() {
        stop();
    }

    void on_message(websocketpp::connection_hdl, ws_client::message_ptr msg) {
        std::string payload = msg->get_payload();
        Json::CharReaderBuilder b;
        std::string errs;
        Value j;
        std::unique_ptr<Json::CharReader> reader(b.newCharReader());
        if (reader->parse(payload.c_str(), payload.c_str() + payload.size(), &j, &errs)) {
            if (callback) callback(j);
        } else {
            std::cerr << "[EventListener] JSON parse error: " << errs << "\n";
        }
    }

    bool start() {
        if (running.exchange(true)) return true;
        try {
            client.set_message_handler([this](websocketpp::connection_hdl h, ws_client::message_ptr msg){
                this->on_message(h, msg);
            });

            websocketpp::lib::error_code ec;
            ws_client::connection_ptr con = client.get_connection(uri, ec);
            if (ec) {
                std::cerr << "[EventListener] get_connection failed: " << ec.message() << "\n";
                running = false;
                return false;
            }
            hdl = con->get_handle();
            client.connect(con);

            runner = std::thread([this](){
                try {
                    client.run();
                } catch (const std::exception &e) {
                    std::cerr << "[EventListener] client.run exception: " << e.what() << "\n";
                }
            });
            return true;
        } catch (const std::exception &e) {
            std::cerr << "[EventListener] start exception: " << e.what() << "\n";
            running = false;
            return false;
        }
    }

    void stop() {
        if (!running.exchange(false)) return;
        try {
            websocketpp::lib::error_code ec;
            client.stop_perpetual();
            client.close(hdl, websocketpp::close::status::normal, "shutdown", ec);
            if (ec) {
                // may fail if not connected
            }
        } catch (...) {}
        if (runner.joinable()) runner.join();
    }
};

EventListener::EventListener(const std::string &controllerWsUrl) : pimpl(new Impl(controllerWsUrl)) {}
EventListener::~EventListener() { delete pimpl; }

bool EventListener::start() { return pimpl->start(); }
void EventListener::stop() { pimpl->stop(); }
void EventListener::setNotificationCallback(std::function<void(const Json::Value&)> cb) { pimpl->callback = cb; }