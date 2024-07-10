#pragma once
#include <compare>

#include <string>

namespace strconsts {

    static const std::string x_start = "x0";
    static const std::string x_end = "x1";
    static const std::string y_start = "y0";
    static const std::string y_end = "y1";
    static const std::string x_offset = "offsetX";
    static const std::string y_offset = "offsetY";
    static const std::string x_str = "x";
    static const std::string y_str = "y";
    static const std::string h_str = "h";
    static const std::string w_str = "w";
}

enum class RequestType {
    API,
    PLAYER,
    FILE
};

struct RequestData {
    RequestType type;
    std::string r_target;
};

namespace util {

/**
 * Вспомогательный шаблонный класс "Маркированный тип".
 * С его помощью можно описать строгий тип на основе другого типа.
 * Пример:
 *
 *  struct AddressTag{}; // метка типа для строки, хранящей адрес
 *  using Address = util::Tagged<std::string, AddressTag>;
 *
 *  struct NameTag{}; // метка типа для строки, хранящей имя
 *  using Name = util::Tagged<std::string, NameTag>;
 *
 *  struct Person {
 *      Name name;
 *      Address address;
 *  };
 *
 *  Name name{"Harry Potter"s};
 *  Address address{"4 Privet Drive, Little Whinging, Surrey, England"s};
 *
 * Person p1{name, address}; // OK
 * Person p2{address, name}; // Ошибка, Address и Name - разные типы
 */

template <typename Value, typename Tag>
class Tagged {
public:
    using ValueType = Value;
    using TagType = Tag;

    explicit Tagged(Value&& v)
        : value_(std::move(v)) {
    }
    explicit Tagged(const Value& v)
        : value_(v) {
    }

    const Value& operator*() const {
        return value_;
    }

    Value& operator*() {
        return value_;
    }

    // Так в C++20 можно объявить оператор сравнения Tagged-типов
    // Будет просто вызван соответствующий оператор для поля value_
    auto operator<=>(const Tagged<Value, Tag>&) const = default;

private:
    Value value_;
};

// Хешер для Tagged-типа, чтобы Tagged-объекты можно было хранить в unordered-контейнерах
template <typename TaggedValue>
struct TaggedHasher {
    size_t operator()(const TaggedValue& value) const {
        // Возвращает хеш значения, хранящегося внутри value
        return std::hash<typename TaggedValue::ValueType>{}(*value);
    }
};

}  // namespace util
