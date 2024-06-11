#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

    void ReportError(beast::error_code ec, std::string_view what) {
        boost::json::object add_data;
        add_data["code"] = ec.value();
        add_data["text"] = ec.message();
        std::string what_str(what);
        add_data["where"] = what_str;
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "error";
        //std::cerr << what << ": "sv << ec.message() << std::endl;
        //std::cerr << "code: " << ec.value() << std::endl;
    }

    void ReportServerExit(int code, const std::exception* ex) {
        boost::json::object add_data;
        add_data["code"] = code;
        if (ex) {
            std::string what_str(ex->what());
            add_data["exception"] = what_str;
        }
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "server exited";
    }

    void SessionBase::Run() {
        net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }

    void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
        if (ec) {
            return ReportError(ec, "write"sv);
        }
        if (close) {
            // Семантика ответа требует закрыть соединение
            return SessionBase::Close();
        }
        // Считываем следующий запрос
        SessionBase::Read();
    }

    void SessionBase::Read() {
        using namespace std::literals;
        // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
        request_ = {};
        stream_.expires_after(30s);
        // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
        http::async_read(stream_, buffer_, request_,
                        // По окончании операции будет вызван метод OnRead
                        beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
    }

    void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
        using namespace std::literals;
        if (ec == http::error::end_of_stream) {
            // Нормальная ситуация - клиент закрыл соединение
            return SessionBase::Close();
        }
        if (ec) {
            return ReportError(ec, "read"sv);
        }
        HandleRequest(std::move(request_), stream_.socket().remote_endpoint().address().to_string());
    }

    void SessionBase::Close() {
        try {
            stream_.socket().shutdown(tcp::socket::shutdown_send);
        } catch (const std::exception& e) {
            std::cerr << "Error closing session: " << e.what() << std::endl;
        }
    }

}  // namespace http_server
