#include <miniatura/http/server.h>

int main() {
	boost::asio::io_context context;
	boost::asio::ip::tcp::resolver resolver(context);
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve("0.0.0.0", "8080").begin();
	boost::asio::ip::tcp::acceptor acceptor(context);
	acceptor.open(endpoint.protocol());
	acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor.bind(endpoint);
	acceptor.listen();

	boost::asio::ip::tcp::socket sock(context);

	miniatura::http::HttpServer server(std::move(acceptor), std::move(sock));

	server.addMethodHandler("GET",
		std::regex("/"),
		[](const miniatura::http::RequestTarget &requestTarget,
			miniatura::http::Response &responseOut) {
			responseOut.statusCode = miniatura::http::HttpStatus::Ok;
			responseOut.headers["Content-Type"] = "text/html";
			responseOut.body = "OK";
		},
		{});

	server.beginAccept();
	context.run();

	return 0;
}
