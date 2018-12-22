#include "NetworkManager.h"

#include "spareio-core/Logger.h"

#include <iostream>
#include <boost/bind.hpp>

NetworkManager::NetworkManager(io_service& io_service, ssl::context& context) :
    m_resolver(io_service),
    m_socket(io_service, context)
{
}

void NetworkManager::post(const std::string& server, const std::string& path, const std::string& json)
{
    std::ostream request_stream(&m_request);

    request_stream << "POST " << path << " HTTP/1.1 \r\n";
    request_stream << "Host: " << server << "\r\n";
    request_stream << "User-Agent: C/1.0";
    request_stream << "Content-Type: application/json; charset=utf-8 \r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Content-Length: " << json.length() << "\r\n";
    request_stream << "Connection: close\r\n\r\n";  //NOTE THE Double line feed
    request_stream << json;

    tcp::resolver::query query(server, "https");
    m_resolver.async_resolve(query,
                             boost::bind(&NetworkManager::handleResolve, this, placeholders::error, placeholders::iterator));
}

void NetworkManager::get(const std::string & server, const std::string & path)
{
    std::ostream request_stream(&m_request);
    request_stream << "GET " << path << " HTTP/1.1\r\n";
    request_stream << "Host: " << server << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    tcp::resolver::query query(server, "https");
    m_resolver.async_resolve(query,
                             boost::bind(&NetworkManager::handleResolve, this, placeholders::error, placeholders::iterator));
}

void NetworkManager::handleResolve(const error_code& err, tcp::resolver::iterator endpoint_iterator)
{
    if (!err)
    {
        Logger::getInstance() << Logger::E_DEBUG << "Resolve OK" << Logger::endl;
        m_socket.set_verify_mode(ssl::verify_peer);
        m_socket.set_verify_callback(boost::bind(&NetworkManager::verifyCertificate, this, _1, _2));

        async_connect(m_socket.lowest_layer(), 
                      endpoint_iterator,
                      boost::bind(&NetworkManager::handleConnect, this, placeholders::error));
    }
    else
    {
        Logger::getInstance() << Logger::E_CRITICAL << "Error resolve: " << err.message() << Logger::endl;
    }
}

bool NetworkManager::verifyCertificate(bool preverified, ssl::verify_context& ctx)
{
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    Logger::getInstance() << Logger::E_INFO << "Verifying " << subject_name << Logger::endl;

    return true;
}

void NetworkManager::handleConnect(const error_code& err)
{
    if (!err)
    {
        Logger::getInstance() << Logger::E_DEBUG << "Connect OK" << Logger::endl;
        m_socket.async_handshake(ssl::stream_base::client, boost::bind(&NetworkManager::handleHandshake, this, placeholders::error));
    }
    else
    {
        Logger::getInstance() << Logger::E_CRITICAL << "Connect failed: " << err.message() << Logger::endl;
    }
}

void NetworkManager::handleHandshake(const error_code& err)
{
    if (!err)
    {
        Logger::getInstance() << Logger::E_DEBUG << "Handshake OK" << Logger::endl;
        Logger::getInstance() << Logger::E_DEBUG << "Request:" << Logger::endl;
        const char* header = buffer_cast<const char*>(m_request.data());
        Logger::getInstance() << Logger::E_DEBUG << header << Logger::endl;

        async_write(m_socket, m_request, boost::bind(&NetworkManager::handleWriteRequest, this, placeholders::error));
    }
    else
    {
        Logger::getInstance() << Logger::E_CRITICAL << "Handshake failed: " << err.message() << Logger::endl;
    }
}

void NetworkManager::handleWriteRequest(const error_code& err)
{
    if (!err)
    {
        boost::asio::async_read_until(m_socket, m_response, "\r\n",
                                      boost::bind(&NetworkManager::handleReadStatusLine, this, placeholders::error));
    }
    else
    {
        Logger::getInstance() << Logger::E_CRITICAL << "Error write req: " << err.message() << Logger::endl;
    }
}

void NetworkManager::handleReadStatusLine(const error_code& err)
{
    if (!err)
    {
        std::istream response_stream(&m_response);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            Logger::getInstance() << Logger::E_CRITICAL << "Invalid response" << Logger::endl;
            return;
        }
        if (status_code != 200)
        {
            Logger::getInstance() << Logger::E_INFO << "Response returned with status code " << status_code << Logger::endl;
            return;
        }
        Logger::getInstance() << Logger::E_INFO << "Status code: " << status_code << Logger::endl;

        async_read_until(m_socket, m_response, "\r\n\r\n",
                         boost::bind(&NetworkManager::handleReadHeaders, this, placeholders::error));
    }
    else
    {
        Logger::getInstance() << Logger::E_CRITICAL << "Error: " << err.message() << Logger::endl;
    }
}

void NetworkManager::handleReadHeaders(const error_code& err)
{
    if (!err)
    {
        std::istream response_stream(&m_response);
        std::string header;
        while (std::getline(response_stream, header) && header != "\r")
        {
            Logger::getInstance() << Logger::E_DEBUG << header << Logger::endl;
        }
        std::cout << "\n";

        if (m_response.size() > 0)
        {
            std::cout << &m_response;
        }

        async_read(m_socket, m_response, transfer_at_least(1),
                   boost::bind(&NetworkManager::handleReadContent, this, placeholders::error));
    }
    else
    {
        Logger::getInstance() << Logger::E_CRITICAL << "Error: " << err.message() << Logger::endl;
    }
}

void NetworkManager::handleReadContent(const boost::system::error_code& err)
{
    if (!err)
    {
        std::cout << &m_response;

        async_read(m_socket, m_response, transfer_at_least(1),
                   boost::bind(&NetworkManager::handleReadContent, this, placeholders::error));
    }
    else if (err != error::eof)
    {
        Logger::getInstance() << Logger::E_CRITICAL << "Error: " << err.message() << Logger::endl;
    }
}
