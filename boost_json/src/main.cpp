#include <iostream>
#include <vector>
#define BOOST_JSON_STACK_BUFFER_SIZE 1024
//#include <boost/json/src.hpp>
#include <boost/json.hpp>

using namespace boost::json;

int main()
{
    std::string js = R"({
    "table":"orderBookL2_25",
    "action":"insert",
    "data":[
        {
            "symbol":"XBTUSD",
            "id":8796514950,
            "side":"Buy",
            "size":10000,
            "price":34850.5
        },
        {
            "symbol":"XBTUSD",
            "id":8796515700,
            "side":"Buy",
            "size":281,
            "price":34843
        }
    ]
})";

    try {
        auto parsed_data = parse(js);
        for (int i = 0; i<parsed_data.at("data").as_array().size(); ++i) {
            std::cout << "id " << (parsed_data.at("data").as_array().at(i).as_object().at("id")) << '\n';
        }
        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}