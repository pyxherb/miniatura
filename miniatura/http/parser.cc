#include "parser.h"

using namespace miniatura;
using namespace miniatura::http;

std::optional<HttpParseError> miniatura::http::parseHttpRequestLine(HttpRequestLine &requestLineOut, std::string_view s) {
	requestLineOut = {};

	size_t i = 0;
	char c;

	while (i < s.size()) {
		c = s[i++];

		switch (c) {
			case ' ':
			case '\t':
				goto methodParseEnd;
			case '\n':
				return HttpParseError{ "Unexpected end of line", 0, i - 1 };
			default:
				requestLineOut.method += c;
		}
	}
	return HttpParseError{ "Prematured end of message", 0, i - 1 };
methodParseEnd:

	while (i < s.size()) {
		switch (s[i]) {
			case ' ':
			case '\t':
				++i;
				break;
			default:
				goto methodWhitespaceParseEnd;
		}
	}
	return HttpParseError{ "Prematured end of message", 0, i - 1 };
methodWhitespaceParseEnd:

	while (i < s.size()) {
		c = s[i++];

		switch (c) {
			case ' ':
			case '\t':
				goto targetParseEnd;
			case '\n':
				return HttpParseError{ "Unexpected end of line", 0, i - 1 };
			default:
				requestLineOut.target += c;
		}
	}
	return HttpParseError{ "Prematured end of message", 0, i - 1 };
targetParseEnd:

	while (i < s.size()) {
		switch (s[i]) {
			case ' ':
			case '\t':
				++i;
				break;
			default:
				goto targetWhitespaceParseEnd;
		}
	}
	return HttpParseError{ "Prematured end of message", 0, i - 1 };
targetWhitespaceParseEnd:

	while (i < s.size()) {
		c = s[i++];

		switch (c) {
			case '\n':
				goto versionParseEnd;
			default:
				requestLineOut.version += c;
		}
	}
	return HttpParseError{ "Prematured end of message", 0, i - 1 };
versionParseEnd:

	return {};
}

std::optional<HttpParseError> miniatura::http::parseHttpStatusLine(HttpStatusLine &statusLineOut, std::string_view s) {
	statusLineOut = {};
	size_t i = 0;
	char c;

	while (i < s.size()) {
		c = s[i++];

		switch (c) {
			case ' ':
			case '\t':
				goto versionParseEnd;
			case '\n':
				return HttpParseError{ "Unexpected end of line", 0, i - 1 };
			default:
				statusLineOut.version += c;
		}
	}
	return HttpParseError{ "Prematured end of message", 0, i - 1 };
versionParseEnd:

	while (i < s.size()) {
		switch (s[i]) {
			case ' ':
			case '\t':
				++i;
				break;
			default:
				goto versionWhitespaceParseEnd;
		}
	}
	return HttpParseError{ "Prematured end of message", 0, i - 1 };
versionWhitespaceParseEnd:

	while (i < s.size()) {
		c = s[i++];

		switch (c) {
			case ' ':
			case '\t':
				goto statusCodeParseEnd;
			case '\n':
				return HttpParseError{ "Unexpected end of line", 0, i - 1 };
			default:
				((uint16_t &)statusLineOut.statusCode) += c;
		}
	}
	return HttpParseError{ "Prematured end of message", 0, i - 1 };
statusCodeParseEnd:

	while (i < s.size()) {
		switch (s[i]) {
			case ' ':
			case '\t':
				++i;
				break;
			default:
				goto statusCodeWhitespaceParseEnd;
		}
	}
	return HttpParseError{ "Prematured end of message", 0, i - 1 };
statusCodeWhitespaceParseEnd:

	statusLineOut.statusText = s.substr(i);
	return {};
}

std::optional<HttpParseError> HttpRequestParser::parseHeaders(std::string_view s) {
	size_t i = 0, line = 0, column = 0;

	std::pair<std::string, std::string> curHeader;
	HeaderState state = HeaderState::Initial;
	char c;

	while (i < s.size()) {
		switch (state) {
			case HeaderState::Initial: {
				c = s[i++];
				++column;

				switch (c) {
					case ':':
						state = HeaderState::HeaderValueWhitespaces;
						break;
					case '\r':
					case '\n':
						if (curHeader.first.empty())
							goto headersParseEnd;
						return HttpParseError{ "Prematured end of line", line, column };
					default:
						curHeader.first += c;
				}
				break;
			}
			case HeaderState::HeaderValueWhitespaces: {
				c = s[i];
				switch (c) {
					case ' ':
					case '\t':
						++i;
						++column;
						break;
					case '\r':
					case '\n':
						return HttpParseError{ "Prematured end of line", line, column };
					default:
						state = HeaderState::HeaderValue;
				}

				break;
			}
			case HeaderState::HeaderValue: {
				c = s[i++];

				switch (c) {
					case '\n':
						headers.insert(curHeader);
						curHeader = {};

						state = HeaderState::Initial;
						++line, column = 0;
						break;
					default:
						curHeader.second += c;
				}
			}
		}
	}

	if (!curHeader.first.empty()) {
		headers.insert(curHeader);
	}

headersParseEnd:;
	return {};
}

std::optional<HttpParseError> HttpRequestParser::parse(char c) {
	switch (parseState) {
		case ParseState::RequestLine:
			switch (c) {
				case '\r':
					parseState = ParseState::RequestLineEnd;
					break;
				case '\n':
					nextLine();
					[[fallthrough]];
				default:
					requestLine += c;
			}
			break;
		case ParseState::RequestLineEnd:
			if (c != '\n')
				return HttpParseError{ "Expecting \\n", line, column };
			parseState = ParseState::RequestHeader;
			nextLine();
			break;
		case ParseState::RequestHeader:
			switch (c) {
				case '\r':
					parseState = ParseState::RequestHeaderEnd;
					break;
				case '\n':
					nextLine();
					[[fallthrough]];
				default:
					requestHeader += c;
			}
			break;
		case ParseState::RequestHeaderEnd: {
			if (c != '\n')
				return HttpParseError{ "Prematured end of message", line, column };
			std::optional<HttpParseError> e = parseHeaders(requestHeader);
			if (e.has_value()) {
				e->line += line;
				return e;
			}
			if (auto it = headers.find("Content-Length"); it != headers.end()) {
				size_t size = 0;

				for (size_t i = 0; i < it->second.size(); ++i) {
					switch ((c = it->second[i])) {
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
							if ((SIZE_MAX / 10) < size)
								return HttpParseError{ "Content-Length is too big", line, column };
							size *= 10;
							size += c - '0';
							break;
						default:
							return HttpParseError{ "Malformed Content-Length data", line, column };
					}
				}

				szExpectedRequestBody = size;
				requestBody.resize(size);
			}
			parseState = ParseState::RequestBody;
			nextLine();
			break;
		}
		case ParseState::RequestBody:
			switch (c) {
				case '\r':
					parseState = ParseState::RequestBodyEnd;
					break;
				case '\n':
					nextLine();
					[[fallthrough]];
				default:
					if (szExpectedRequestBody) {
						if (szRequestBodyReceived >= szExpectedRequestBody.value()) {
							parseState = ParseState::End;
						} else {
							requestBody[szRequestBodyReceived++] = c;
							if (szRequestBodyReceived >= szExpectedRequestBody.value()) {
								parseState = ParseState::End;
							}
						}
					} else {
						requestBody += c;
					}
			}
			break;
		case ParseState::RequestBodyEnd:
			if (c != '\n')
				return HttpParseError{ "Prematured end of message", line, column };
			parseState = ParseState::End;
			break;
		case ParseState::End:
			break;
		default:
			assert(false);
	}

	return {};
}
