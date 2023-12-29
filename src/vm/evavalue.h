#pragma once

#include <string>
#include <vector>

enum class EvaValueType {
    NUMBER,
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
        Object *object;
    };

    double asNumber();
    Object *asObject();
    StringObject *asString();
    std::string asCppString();
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

inline bool isNumber(EvaValue val)
{
    return val.type == EvaValueType::NUMBER;
}

inline bool isObject(EvaValue val)
{
    return val.type == EvaValueType::OBJECT;
}

inline bool isObjectType(EvaValue val, ObjectType type)
{
    return isObject(val) && val.asObject()->type == type;
}

inline EvaValue allocString(std::string str)
{
    return EvaValue{.type = EvaValueType::OBJECT, .object = new StringObject(std::move(str))};
}

#define NUMBER(x) EvaValue({.type = EvaValueType::NUMBER, .number = x})
#define STRING(x) EvaValue({.type = EvaValueType::OBJECT, .object = new StringObject(x)})
