#pragma once

#include "spareio-core/TelemetryHelper.h"

#include <string>

class NetworkManagerHelper
{
public:
    static void get(const std::string& server, const std::string& path);
    static void post(const std::string& server, const std::string& path, const std::string& json);
    static void sendEvent(const std::string& workerId, TelemetryHelper::Event e, const std::string& message = {});
};