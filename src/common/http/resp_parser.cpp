#include "resp_parser.h"

/*
 * The parsing automata. Generated by python scripts.
 */
#include <http_resp_automaton.h>

using automata::Event;
using automata::ExecutionControl;
using con::parser::response::Names;
using http::ContentType;

namespace {

const automata::Automaton http_response(con::parser::response::paths, con::parser::response::transitions, con::parser::response::states);

}

namespace http::parser {

ResponseParser::ResponseParser(ExtraHeader *extra_hdr)
    : Execution(&http_response)
    , extra_hdr(extra_hdr)
    , version_major(0)
    , version_minor(0) {}

ExecutionControl ResponseParser::event(Event event) {
    switch (event.entering_state) {
    case Names::StatusCode:
        status_code = 10 * status_code + (event.payload - '0');
        return ExecutionControl::Continue;
    case Names::Body:
        done = true;
        // Yes, really don't stop. Eath the \n too!
        return ExecutionControl::Continue;
    case Names::ContentLength:
        if (!content_length.has_value()) {
            content_length = 0;
        }
        *content_length = 10 * *content_length + (event.payload - '0');
        return ExecutionControl::Continue;
    case Names::ApplicationJson:
        content_type = ContentType::ApplicationJson;
        return ExecutionControl::Continue;
    case Names::TextGcodeDot:
        content_type = ContentType::TextGcode;
        return ExecutionControl::Continue;
    case Names::TextGcodeDash:
        content_type = ContentType::TextGcode;
        return ExecutionControl::Continue;
    case Names::CommandId:
        extra(event.payload, HeaderName::CommandId);
        return ExecutionControl::Continue;
    case Names::Code:
        extra(event.payload, HeaderName::Code);
        return ExecutionControl::Continue;
    case Names::Token:
        extra(event.payload, HeaderName::Token);
        return ExecutionControl::Continue;
    case Names::ConnectionHeader:
        // This comes when we see a connection header. If we understand it and it's keep-alive one, we amend it.
        keep_alive = false;
        return ExecutionControl::Continue;
    case Names::ConnectionKeepAlive:
        keep_alive = true;
        return ExecutionControl::Continue;
    case Names::Version:
        switch (event.payload) {
        case '.':
            version_major = version_minor;
            version_minor = 0;
            return ExecutionControl::Continue;
        case '0' ... '9':
            version_minor = 10 * version_minor + (event.payload - '0');
            return ExecutionControl::Continue;
        }
        [[fallthrough]];
    default:
        return ExecutionControl::Continue;
    }
}

void ResponseParser::extra(char c, HeaderName name) {
    if (extra_hdr != nullptr) {
        extra_hdr->character(c, name);
    }
}

}
