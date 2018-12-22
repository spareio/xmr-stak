#include "NetworkManagerHelper.h"

#include "spareio-core/NetworkManager.h"

void NetworkManagerHelper::get(const std::string& server, const std::string& path)
{
    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    ctx.set_default_verify_paths();

    boost::asio::io_service io_service;
    NetworkManager c(io_service, ctx);
    c.get(server, path);
    io_service.run();
}

void NetworkManagerHelper::post(const std::string& server, const std::string& path, const std::string& json)
{
    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    ctx.set_default_verify_paths();

    boost::asio::io_service io_service;
    NetworkManager c(io_service, ctx);
    c.post(server, path, json);
    io_service.run();
}

void NetworkManagerHelper::sendEvent(const std::string& workerId, TelemetryHelper::Event e, const std::string& message)
{
    post(TelemetryHelper::getInstance().getTelemetryServer(),
         TelemetryHelper::INFO_HYPERVISOR_PATH,
         TelemetryHelper::createEventJson(workerId, e, message));
}
