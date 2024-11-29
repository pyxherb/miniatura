#include "server.h"

using namespace miniatura;
using namespace miniatura::http;

MINIATURA_API HttpSession::HttpSession(HttpServer *httpServer, boost::asio::ip::tcp::socket &&socket)
	: httpServer(httpServer), socket(std::move(socket)) {
}

MINIATURA_API HttpSession::~HttpSession() {
}

MINIATURA_API void HttpSession::stop() {
	down = true;
}

MINIATURA_API HttpServer::HttpServer(
	boost::asio::ip::tcp::acceptor &&acceptor,
	boost::asio::ip::tcp::socket &&socket)
	: acceptor(std::move(acceptor)),
	  socket(std::move(socket)) {
}

MINIATURA_API void HttpServer::run() {
}

MINIATURA_API void HttpSession::read() {
	auto self(shared_from_this());
	socket.async_read_some(
		boost::asio::buffer(buf),
		[this, self](const boost::system::error_code errorCode, size_t szRead) {
			if (errorCode == boost::asio::error::operation_aborted) {
				terminate();
			}

			if (!errorCode) {
				for (size_t i = 0; i < szRead; ++i) {
					auto e = parser.parse(buf[i]);
					if (e.has_value()) {
						terminate();
					}

					if (parser.parseState == HttpRequestParser::ParseState::End) {
						break;
					}
				}

				if (parser.parseState == HttpRequestParser::ParseState::End) {
					parser.reset();

					self->writeBuffer =
						"HTTP/1.1 200 OK\r\n"
						"Content-Type: text/html\n"
						"Content-Length: 22\r\n\r\n"
						"<h1>Hello, World!</h1>\r\n";

					write();
				} else {
					read();
				}
			} else {
				printf("%s\n", errorCode.message().c_str());
			}
		});
}

MINIATURA_API void HttpSession::write() {
	auto self(shared_from_this());
	socket.async_write_some(
		boost::asio::buffer(writeBuffer),
		[this, self](const boost::system::error_code errorCode, size_t szWritten) {
			if (errorCode) {
				terminate();
			}

			self->writeBuffer.clear();
		});
}

MINIATURA_API void HttpServer::beginAccept() {
	// acceptor.async_accept(socket, std::bind(&HttpServer::endAccept, this, session, boost::asio::placeholders::error));
	acceptor.async_accept([this](boost::system::error_code errorCode, boost::asio::ip::tcp::socket socket) {
		if (!acceptor.is_open())
			return;
		if (errorCode)
			terminate();
		std::shared_ptr<HttpSession> session = std::make_shared<HttpSession>(this, std::move(socket));
		session->read();
		beginAccept();
	});
}

MINIATURA_API void HttpServer::endAccept(std::shared_ptr<HttpSession> session, boost::system::error_code errorCode) {
	if (errorCode) {
		terminate();
	}

	session->read();

	beginAccept();
}
