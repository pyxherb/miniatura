#ifndef _MINIATURA_SERVER_H_
#define _MINIATURA_SERVER_H_

#include "parser.h"
#include <boost/asio.hpp>
#include <regex>
#include <set>

namespace miniatura {
	namespace http {
		class HttpServer;

		class HttpSession : public std::enable_shared_from_this<HttpSession> {
		public:
			HttpServer *httpServer;
			char buf[4096] = { 0 };
			bool down = false;
			HttpRequestParser parser;
			std::string writeBuffer;
			boost::asio::ip::tcp::socket socket;

			MINIATURA_API HttpSession(HttpServer *httpServer, boost::asio::ip::tcp::socket &&socket);
			MINIATURA_API ~HttpSession();

			MINIATURA_API void stop();

			MINIATURA_API void read();
			MINIATURA_API void write();
		};

		struct RequestTarget {
			std::string_view version;
			std::string_view methodName;
			std::string path;
			std::string parameters;
			std::string fragment;
			std::smatch matchResults;
		};

		struct Response {
			std::string version;
			HttpStatus statusCode;
			std::string statusText;
			std::unordered_map<std::string, std::string> headers;
			std::string body;
		};

		using HttpHandler = std::function<void (const RequestTarget &requestTarget, Response &responseOut)>;

		struct HttpHandlerAttribute {
			bool noCaptureGroup : 1;
		};

		struct HttpHandlerStorage {
			std::regex regexp;
			HttpHandler handler;
			HttpHandlerAttribute attributes;
		};

		struct MethodRegistry {
			std::list<HttpHandlerStorage> handlers;
		};

		class HttpServer {
		public:
			boost::asio::ip::tcp::acceptor acceptor;
			boost::asio::ip::tcp::socket socket;

			std::set<std::shared_ptr<HttpSession>> sessions;
			std::unordered_map<std::string_view, MethodRegistry> methodRegistries = {
				{ "GET", {} },
				{ "POST", {} },
				{ "PUT", {} },
				{ "HEAD", {} },
				{ "DELETE", {} },
				{ "OPTIONS", {} },
				{ "TRACE", {} },
				{ "CONNECT", {} },
				{ "PATCH", {} }
			};

			MINIATURA_API HttpServer(
				boost::asio::ip::tcp::acceptor &&acceptor,
				boost::asio::ip::tcp::socket &&socket);

			MINIATURA_API void run();

			MINIATURA_API void beginAccept();
			MINIATURA_API void endAccept(std::shared_ptr<HttpSession> session, boost::system::error_code errorCode);

			MINIATURA_API void addMethodHandler(
				std::string_view method,
				std::regex &&regexp,
				HttpHandler &&handler,
				HttpHandlerAttribute attributes);
		};
	}
}

#endif
