#pragma once

#include <string>

enum class EvaValueType {
    NUMBER,
    OBJECT,
};

enum class ObjectType {
    STRING,
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

struct EvaValue {
    EvaValueType type;
    union {
        double number;
        Object *object;
    };

    double asNumber() { return this->number; }

    Object *asObject() { return this->object; }

    StringObject *asString()
    {
        if (type == EvaValueType::OBJECT && object->type == ObjectType::STRING) {
            return (StringObject *) object;
        }
        return nullptr;
    }

    std::string asCppString()
    {
        if (type == EvaValueType::OBJECT && object->type == ObjectType::STRING) {
            auto sobj = (StringObject *) object;
            return sobj->string;
        } else {
            return "";
        }
    }
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
    return EvaValue{EvaValueType::OBJECT, .object = new StringObject(std::move(str))};
}

#define NUMBER(x) EvaValue({EvaValueType::NUMBER, .number = x})
#define STRING(x) EvaValue({EvaValueType::OBJECT, .object = new StringObject(x)})
