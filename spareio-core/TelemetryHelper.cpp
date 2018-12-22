#include "TelemetryHelper.h"

#include "spareio-core/TimeZone.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;

const std::string TelemetryHelper::TELEMETRY_DEVELOPMENT_SERVER = "he.devspare.io";
const std::string TelemetryHelper::TELEMETRY_PRODUCTION_SERVER = "he.spare.io";
const std::string TelemetryHelper::INFO_HYPERVISOR_PATH = "/he/v1/event?Type=InfoHypervisor&EventVersion=1.1";

TelemetryHelper& TelemetryHelper::getInstance()
{
    static TelemetryHelper instance;
    return instance;
}

void TelemetryHelper::setProduction(bool isProduction)
{
    m_isProduction = isProduction;
}

bool TelemetryHelper::isProduction() const
{
    return m_isProduction;
}

std::string TelemetryHelper::getTelemetryServer()
{
    return m_isProduction ? TELEMETRY_PRODUCTION_SERVER : TELEMETRY_DEVELOPMENT_SERVER;
}

std::string TelemetryHelper::createEventJson(const std::string& workerId, TelemetryHelper::Event e, const std::string& message)
{
    ptree root, data;
    data.put("workerId", workerId);
    data.put("hvVersion", "1.0.0.0");
    data.put("systemTime", TimeZone::getUtcDateTime_ms());
    data.put("message", TelemetryHelper::getStandardMessage(e, "Digger: ") + "\n" + message);
    root.put_child("Data", data);

    std::ostringstream buf;
    write_json(buf, root, false);

    return buf.str();
}

std::string TelemetryHelper::getStandardMessage(TelemetryHelper::Event e, const std::string& prefix)
{
    static_assert(TelemetryHelper::Event::E_EVENT__END == static_cast<TelemetryHelper::Event>(7), "Need to implement method for new event");

    std::string message = [&e]()
    {
        switch (e) 
        {
        case TelemetryHelper::Event::E_STARTED:
            return "Started";
        case TelemetryHelper::Event::E_WORKER_ID_IS_NO_SET:
            return "Worker id is not set";
        case TelemetryHelper::Event::E_ONLINE:
            return "Online";
        case TelemetryHelper::Event::E_CRASHED:
            return "Crash";
        case TelemetryHelper::Event::E_BKEY_MATCH:
            return "bKey Match";
        case TelemetryHelper::Event::E_BKEY_FAIL:
            return "bKey Fail";
        case TelemetryHelper::Event::E_CPU_CONFIGURATION_INFO:
            return "Cpu: ";
        case TelemetryHelper::Event::E_OFF:
            return "Off";
        default:
            return "Not implemented event";
        }
    }();

    return prefix + message;
}
