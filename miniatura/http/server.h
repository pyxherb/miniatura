#ifndef _MINIATURA_SERVER_H_
#define _MINIATURA_SERVER_H_

#include "parser.h"
#include <boost/asio.hpp>
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

		class HttpServer {
		public:
			boost::asio::ip::tcp::acceptor acceptor;
			boost::asio::ip::tcp::socket socket;

			std::set<std::shared_ptr<HttpSession>> sessions;

			MINIATURA_API HttpServer(
				boost::asio::ip::tcp::acceptor &&acceptor,
				boost::asio::ip::tcp::socket &&socket);

			MINIATURA_API void run();

			MINIATURA_API void beginAccept();
			MINIATURA_API void endAccept(std::shared_ptr<HttpSession> session, boost::system::error_code errorCode);
		};
	}
}

#endif
