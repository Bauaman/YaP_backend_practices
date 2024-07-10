#include "logger.h"

namespace logger {

void Logger::Init() {
    logging::add_common_attributes();
    logging::add_console_log(
        std::cout,
        keywords::auto_flush = true,
        keywords::format = &MyFormatter
    );
}

void Logger::MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto ts = *rec[timestamp];
    auto data = *rec[additional_data];
    auto msg = rec[logging::expressions::message];
    boost::json::object message;
    message["timestamp"] = to_iso_extended_string(ts);
    boost::json::object data_obj;
    if (data.is_object()) {
        for (const auto& pair : data.as_object()) {
            data_obj[pair.key()] = pair.value();
        }
    }
    message["data"] = data_obj;
    message["message"] = msg.get<std::string>();
    strm << boost::json::serialize(message);
}

void LogExit(const int code, const std::exception* ex) {
    boost::json::object add_data;
    add_data["code"] = code;
    if (ex) {
        std::string what_str(ex->what());
        add_data["exception"] = what_str;
    }
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "server exited";
}

void LogMessageInfo (const boost::json::value& add_data, const std::string message) {
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << message;
}

void LogError(const sys::error_code& ec, std::string_view where) {
    boost::json::object message{
        {"code", ec.value()},
        {"text", ec.message()},
        {"where", where}};
    BOOST_LOG_TRIVIAL(error) << logging::add_value(additional_data, message) << "error"sv;
}

void LogError(const std::exception& ex) {
    boost::json::object obj{{"exception", ex.what()}};
    BOOST_LOG_TRIVIAL(error) << boost::log::add_value(additional_data, obj) << "error"sv;
}

}