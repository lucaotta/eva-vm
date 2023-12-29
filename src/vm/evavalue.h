#pragma once

#include <string>
#include <vector>

enum class EvaValueType {
    NUMBER,
    BOOL,
    OBJECT,
};

enum class ObjectType {
    STRING,
    CODE,
};

struct Object;
struct StringObject;
struct CodeObject;

struct EvaValue
{
    EvaValueType type;
    union {
        double number;
        bool boolean;
        Object *object;
    };

    double asNumber() const;
    bool asBool() const;
    Object *asObject() const;
    StringObject *asString() const;
    std::string asCppString() const;
    CodeObject *asCodeObject() const;
};

struct Object
{
    Object(ObjectType type)
        : type(type)
    {}

    ObjectType type;
};

struct StringObject : public Object
{
    StringObject(std::string str)
        : Object(ObjectType::STRING)
        , string(std::move(str))
    {}

    std::string string;
};

struct CodeObject : public Object
{
    CodeObject(std::string name)
        : Object(ObjectType::CODE)
        , name(std::move(name))
    {}

    std::string name;
    std::vector<uint8_t> code;
    std::vector<EvaValue> constants;
};

inline bool isNumber(const EvaValue &val)
{
    return val.type == EvaValueType::NUMBER;
}
inline bool isBool(const EvaValue &val)
{
    return val.type == EvaValueType::BOOL;
}

inline bool isObject(const EvaValue &val)
{
    return val.type == EvaValueType::OBJECT;
}

inline bool isObjectType(const EvaValue &val, ObjectType type)
{
    return isObject(val) && val.asObject()->type == type;
}

inline bool isString(const EvaValue &val)
{
    return isObjectType(val, ObjectType::STRING);
}

inline EvaValue allocString(std::string str)
{
    return EvaValue{.type = EvaValueType::OBJECT, .object = new StringObject(std::move(str))};
}

inline EvaValue allocCode(std::string name)
{
    return EvaValue{.type = EvaValueType::OBJECT, .object = new CodeObject(std::move(name))};
}

inline std::string toString(const EvaValue &value)
{
    if (isNumber(value)) {
        return std::to_string(value.asNumber());
    }
    if (isString(value)) {
        return value.asString()->string;
    }
    if (isBool(value)) {
        return std::to_string(value.asBool());
    }
    return "";
}

#define NUMBER(x) EvaValue({.type = EvaValueType::NUMBER, .number = x})
#define BOOLEAN(x) EvaValue({.type = EvaValueType::BOOL, .boolean = x})
