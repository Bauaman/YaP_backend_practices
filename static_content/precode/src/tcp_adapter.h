#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class AsyncWriteTcpStreamAdapter {
public:
    explicit AsyncWriteTcpStreamAdapter(boost::beast::tcp_stream& stream) : stream_(stream) {
    }

    template <typename ConstBufferSequence>
    void write_some(const ConstBufferSequence& cbs, std::function<void(const boost::system::error_code&, size_t)> handler) {
        size_t total_size = net::buffer_size(cbs);
        if (total_size == 0) {
            handler({}, 0);
            return;
        }

        stream_.async_write_some(cbs, [handler](const boost::system::error_code& ec, size_t bytes_transferred) {
            handler(ec, bytes_transferred);
        });
    }

    template <typename ConstBufferSequence>
    void write_some(const ConstBufferSequence& cbs) {
        write_some(cbs, [](const boost::system::error_code& ec, size_t bytes_transferred) {
            if (ec) {
                throw std::runtime_error("Failed to write");
            }
        });
    }

private:
    boost::beast::tcp_stream& stream_;
};