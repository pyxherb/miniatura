#ifndef _MINIATURA_PARSER_H_
#define _MINIATURA_PARSER_H_

#include <miniatura/basedefs.h>
#include <optional>
#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace miniatura {
	namespace http {
		enum class HttpStatus : uint16_t {
			Ok = 200,
			Created = 201,
			Accepted = 202,
			NoContent = 204,
			MultipleChoices = 300,
			MovedPermanently = 301,
			MovedTemporarily = 302,
			NotModified = 304,
			BadRequest = 400,
			Unauthorized = 401,
			Forbidden = 403,
			NotFound = 404,
			InternalServerError = 500,
			NotImplemented = 501,
			BadGateway = 502,
			ServiceUnavailable = 503
		};

		struct HttpRequestLine {
			std::string method;
			std::string target;
			std::string version;
		};

		struct HttpStatusLine {
			std::string version;
			HttpStatus statusCode;
			std::string statusText;
		};

		struct HttpParseError {
			const char *message;
			size_t line;
			size_t column;
		};

		std::optional<HttpParseError> parseHttpRequestLine(HttpRequestLine &requestLineOut, std::string_view s);
		std::optional<HttpParseError> parseHttpStatusLine(HttpStatusLine &statusLineOut, std::string_view s);

		class HttpRequestParser {
		public:
			std::string requestLine, requestHeader, requestBody;
			std::unordered_map<std::string, std::string> headers;
			std::optional<size_t> szExpectedRequestBody;
			size_t szRequestBodyReceived = 0;

			size_t line = 0;
			size_t column = 0;

			enum class ParseState : uint8_t {
				RequestLine = 0,
				RequestLineEnd,
				RequestHeader,
				RequestHeaderEnd,
				RequestBody,
				RequestBodyEnd,
				End
			};

			enum class HeaderState : uint8_t {
				Initial = 0,
				HeaderValueWhitespaces,
				HeaderValue
			};

			ParseState parseState = ParseState::RequestLine;

			MINIATURA_FORCEINLINE void nextLine() {
				++line;
				column = 0;
			}

			MINIATURA_FORCEINLINE void reset() {
				requestLine.clear();
				requestHeader.clear();
				requestBody.clear();
				headers.clear();
				szExpectedRequestBody.reset();
				szRequestBodyReceived = 0;
				line = 0;
				column = 0;
			}

			std::optional<HttpParseError> parseHeaders(std::string_view s);
			std::optional<HttpParseError> parse(char c);
		};

		const char *getHttpStatusText(HttpStatus status);
	}
}

#endif
