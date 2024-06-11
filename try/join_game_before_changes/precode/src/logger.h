#include <boost/json.hpp>
#include <boost/date_time.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)

class Logger {
public:
    void InitLogging(std::string filename) {
        logging::add_common_attributes();
        /*g_file_sink = logging::add_file_log(
            keywords::file_name = filename,
            keywords::format = &MyFormatter,
            keywords::open_mode = std::ios_base::app | std::ios_base::out
        );*/
        logging::add_console_log(std::cout,
            keywords::format = &MyFormatter,
            keywords::auto_flush = true
        );
    }

    void StopFileLogging() {
        logging::core::get()->remove_sink(g_file_sink);
        g_file_sink.reset();
    }

private:
    typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
    boost::shared_ptr<sink_t> g_file_sink;

    static void MyFormatter (logging::record_view const& rec, logging::formatting_ostream& strm) {
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
        /*strm << rec[line_id] << ": ";
        auto ts = *rec[timestamp];
        strm << to_iso_extended_string(ts) << ": ";
        strm << "<" << rec[logging::trivial::severity] << "> ";
        strm << rec[logging::expressions::message];*/
    }

};

