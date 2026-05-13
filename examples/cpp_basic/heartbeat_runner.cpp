#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include "fountain/fountain.h"

namespace {

std::atomic<bool> g_running(true);

void HandleSignal(int /*signal_number*/) {
    g_running = false;
}

bool ParseArgValue(int argc, char **argv, const std::string &name, std::string &value) {
    bool found = false;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == name) {
            value = argv[i + 1];
            found = true;
        }
    }
    return found;
}

}  // namespace

int main(int argc, char **argv) {
    int exit_code = 1;
    std::string database_path = ".heartbeat/heartbeat.sqlite3";
    std::string install_id;
    std::string interval_seconds = "900";
    std::string event_name = "fountain.heartbeat";
    std::string component = "fountain.runtime";

    ParseArgValue(argc, argv, "--database-path", database_path);
    ParseArgValue(argc, argv, "--interval-seconds", interval_seconds);
    ParseArgValue(argc, argv, "--event-name", event_name);
    ParseArgValue(argc, argv, "--component", component);
    const bool has_install_id = ParseArgValue(argc, argv, "--install-id", install_id);
    const bool has_required_values = has_install_id && !install_id.empty() && !database_path.empty();
    if (!has_required_values) {
        std::cerr << "missing required args: --install-id and --database-path\n";
    } else if (!FountainConfigure(database_path.c_str())) {
        std::cerr << "failed to configure fountain at " << database_path << "\n";
    } else {
        FountainSetInstallID(install_id.c_str());
        FountainHeartbeatConfig config {};
        config.interval_seconds = static_cast<uint32_t>(std::stoul(interval_seconds));
        config.event_name = event_name.c_str();
        config.component = component.c_str();
        config.target_install_id = install_id.c_str();
        const bool started = FountainStartHeartbeat(&config);
        if (!started) {
            std::cerr << "failed to start heartbeat thread\n";
        } else {
            FountainLogField field {};
            field.key = "startup_bootstrap";
            field.privacy = FountainLogPrivacyPublic;
            field.type = FountainLogValueBool;
            field.value.bool_value = true;
            FountainLogEvent(FountainLogLevelInfo, event_name.c_str(), component.c_str(), &field, 1);
            std::signal(SIGINT, HandleSignal);
            std::signal(SIGTERM, HandleSignal);
            while (g_running.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            FountainStopHeartbeat();
            exit_code = 0;
        }
    }
    return exit_code;
}
