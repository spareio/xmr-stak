#pragma once

#include <string>

class TelemetryHelper
{
public:
    static TelemetryHelper& getInstance();

    enum class Event
    {
        E_EVENT__BEGIN = 0,
            E_STARTED = E_EVENT__BEGIN,
            E_WORKER_ID_IS_NO_SET,
            E_ONLINE,
            E_CRASHED,
            E_BKEY_MATCH,
            E_BKEY_FAIL, 
            E_CPU_CONFIGURATION_INFO,
            E_OFF,
        E_EVENT__END = E_OFF,
    };

    static std::string createEventJson(const std::string& workerId, TelemetryHelper::Event e, const std::string& message = {});
    static std::string getStandardMessage(TelemetryHelper::Event e, const std::string& prefix = {});

    static const std::string TELEMETRY_DEVELOPMENT_SERVER;
    static const std::string TELEMETRY_PRODUCTION_SERVER;
    static const std::string INFO_HYPERVISOR_PATH;

    void setProduction(bool isProduction);
    bool isProduction() const;

    std::string getTelemetryServer();

private:
    TelemetryHelper() {};
    TelemetryHelper(const TelemetryHelper& other) = delete;
    TelemetryHelper& operator=(const TelemetryHelper&) = delete;

    bool m_isProduction{ true };
};
