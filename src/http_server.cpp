#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

    void ReportError(beast::error_code ec, std::string_view what) {
        std::string what_str(what);
        logger::LogError(ec, what_str);
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
            std::cout << "EC in OnRead" << std::endl;
            return ReportError(ec, "read"sv);
        }
        HandleRequest(std::move(request_));
    }

    void SessionBase::Close() {
        try {
            stream_.socket().shutdown(tcp::socket::shutdown_send);
            //std::cout << "Session Closed" << std::endl;
        } catch (const std::exception& ex) {
            //std::cerr << "Error closing session: " << e.what() << std::endl;
            logger::LogError(ex);
        }
    }

}  // namespace http_server
