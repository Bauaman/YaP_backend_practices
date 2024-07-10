#pragma once

#include <boost/beast/http.hpp>
#include <boost/date_time.hpp>
#include <boost/json.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>


BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value);
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime);

using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;

namespace logging = boost::log;
namespace keywords = logging::keywords;

namespace logger {

void LogExit(const int code, const std::exception* ex = nullptr);
void LogMessageInfo (const boost::json::value& add_data, const std::string message);
void LogError(const sys::error_code& ec, std::string_view where);
void LogError(const std::exception& ex);

class Logger {
public:
    static void Init();

private:
    static void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);
};

}