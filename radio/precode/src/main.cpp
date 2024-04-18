#include "audio.h"
#include <boost/asio.hpp>
#include <atomic>
#include <iostream>

namespace net = boost::asio;
using net::ip::udp;
using namespace std::literals;

void StartServer (uint16_t port) {
    static const size_t max_buffer_size = 65000;
    boost::asio::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(),port));
    std::array<char, max_buffer_size> recv_buf;
    while (true) {
        
        udp::endpoint sender_endpoint;

        auto size = socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);
        //std::cout << "size: "sv << size << std::endl;
        //std::cout << "Client said: "sv << std::string_view(recv_buf.data(), size) << std::endl;
        Player player(ma_format_u8, 1);
        size_t frame_size = player.GetFrameSize();
        size_t frames = recv_buf.size() / frame_size;
        player.PlayBuffer(recv_buf.data(), frames, 1.5s);
        //std::cout << "Playing done" << std::endl;
    }
}

void StartClient (net::ip::address address, uint16_t port) {
    //Запись с микрофона

    Recorder recorder(ma_format_u8, 1);
    auto rec_result = recorder.Record(65000, 1.5s);
    size_t bytes_recorded = rec_result.data.size();
    size_t frame_size = recorder.GetFrameSize();
    size_t total_bytes_recorded = bytes_recorded * frame_size;
    std::string rec_buf(rec_result.data.begin(), rec_result.data.end());
    //std::cout << total_bytes_recorded << " " << rec_buf.size() << std::endl;
    
    //Отправка сообщения
    net::io_context io_context;
    udp::socket socket(io_context, udp::v4());
    auto endpoint = udp::endpoint(address, port);
    //std::string input_buf;
    //std::getline(std::cin, input_buf);
    socket.send_to(net::buffer(rec_buf), endpoint);
}

int main(int argc, char** argv) {

    int port;


    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " server/client" << std::endl;
        return 1;
    }
    if (argc == 3 && strcmp(argv[1], "client") == 0) {
        std::cout << "Please specify Server IP address and port" << std::endl;
        return 1;
    }
    if (argc == 3 && strcmp(argv[1], "server") == 0) {
        if (atoi(argv[2]) != 0) {
            port = atoi(argv[2]);
        } else {
            std::cout << "Please specify valid port" << std::endl;
            return 1;
        }
        try {
            StartServer (port);
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }
    if (argc == 4 && strcmp(argv[1], "client") == 0) {
        if (atoi(argv[3]) != 0) {
            port = atoi(argv[3]);
        } else {
            std::cout << "Please specify valid port" << std::endl;
            return 1;
        }
        try {
            boost::system::error_code ec;
            net::ip::address server_ip = net::ip::make_address(argv[2], ec);
            StartClient(server_ip, port);
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
        /*
        while (true) {
        std::string str;

        std::cout << "Press Enter to record message..." << std::endl;
        std::getline(std::cin, str);

        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording done" << std::endl;

        player.PlayBuffer(rec_result.data.data(), rec_result.frames, 1.5s);
        std::cout << "Playing done" << std::endl;
        */
    
    return 0;
}
