#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using tcp = boost::asio::ip::tcp;
using error_code = boost::system::error_code;
using namespace boost::asio;

class NetworkManager
{
public:
	explicit NetworkManager(io_service& io_service, ssl::context& context);

    void get(const std::string& server, const std::string& path);
    void post(const std::string& server, const std::string& path, const std::string& json);
     
private:
    void handleResolve(const error_code& err, tcp::resolver::iterator endpoint_iterator);
    bool verifyCertificate(bool preverified, ssl::verify_context& ctx);
    void handleConnect(const error_code& err);
    void handleHandshake(const error_code& err);
    void handleWriteRequest(const error_code& err);
    void handleReadStatusLine(const error_code& err);
    void handleReadHeaders(const error_code& err);
    void handleReadContent(const error_code& err);

    tcp::resolver m_resolver;
    ssl::stream<tcp::socket> m_socket;
    streambuf m_request;
    streambuf m_response;
};