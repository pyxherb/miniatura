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
					HttpRequestLine requestLine;
					std::unordered_map<std::string, std::string> headers = std::move(parser.headers);
					std::string body = std::move(parser.requestBody);

					Response responseOut;
					responseOut.version = "1.1";

					if (auto e = parseHttpRequestLine(requestLine, parser.requestLine); e) {
						responseOut.statusCode = HttpStatus::BadRequest;
						goto succeeded;
					}

					{
						RequestTarget requestTargetOut;

						requestTargetOut.version = requestLine.version;
						requestTargetOut.methodName = requestLine.method;
						requestTargetOut.targetName = requestLine.target;

						if (auto it = httpServer->methodRegistries.find(requestLine.method); it != httpServer->methodRegistries.end()) {
							MethodRegistry &registry = it->second;

							for (auto &i : registry.handlers) {
								if (i.attributes.noCaptureGroup) {
									if (std::regex_search(requestLine.target, i.regexp)) {
										i.handler(requestTargetOut, responseOut);
										goto succeeded;
									}
								} else {
									if (std::regex_match(requestLine.target, requestTargetOut.matchResults, i.regexp)) {
										i.handler(requestTargetOut, responseOut);
										goto succeeded;
									}
								}
							}
						} else {
							terminate();
						}
					}

				succeeded:
					self->writeBuffer = "HTTP/" + std::move(responseOut.version) + " ";
					self->writeBuffer += std::to_string((uint16_t)responseOut.statusCode) + " ";
					if (responseOut.statusText.empty())
						responseOut.statusText = getHttpStatusText(responseOut.statusCode);
					self->writeBuffer += std::move(responseOut.statusText) + "\r\n";

					std::optional<size_t> bodyLength;
					for (auto &i : responseOut.headers) {
						if (i.first == "Content-Length") {
							for (size_t j = 0; j < i.second.size(); ++j) {
								bodyLength = 0;
								switch (char c = i.second[j]; c) {
									case '0':
									case '1':
									case '2':
									case '3':
									case '4':
									case '5':
									case '6':
									case '7':
									case '8':
									case '9':
										bodyLength.value() += c - '0';
										break;
									default:
										assert(("Invalid Content-Length data", false));
								}
							}
						}
						self->writeBuffer += i.first + ": " + i.second + "\r\n";
					}
					responseOut.headers.clear();
					if (!bodyLength.has_value()) {
						self->writeBuffer += "Content-Length: " + std::to_string(responseOut.body.size()) + "\r\n";
					}

					self->writeBuffer += "\r\n";

					self->writeBuffer += std::move(responseOut.body);

					self->writeBuffer += "\r\n";

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
	parser.reset();
	boost::asio::async_write(
		socket,
		boost::asio::buffer(writeBuffer),
		[this, self](const boost::system::error_code errorCode, size_t szWritten) {
			if (errorCode) {
				terminate();
			}

			boost::system::error_code ignoredErrorCode;
			socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignoredErrorCode);
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

MINIATURA_API void HttpServer::addMethodHandler(
	std::string_view method,
	std::regex &&regexp,
	HttpHandler &&handler,
	HttpHandlerAttribute attributes) {
	MethodRegistry &methodRegistry = methodRegistries.at(method);

	methodRegistry.handlers.push_front({ std::move(regexp), std::move(handler), attributes });
}
