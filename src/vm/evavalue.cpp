#include "evavalue.h"

double EvaValue::asNumber() const
{
    return this->number;
}

Object *EvaValue::asObject() const
{
    return this->object;
}

StringObject *EvaValue::asString() const
{
    if (type == EvaValueType::OBJECT && object->type == ObjectType::STRING) {
        return (StringObject *) object;
    }
    return nullptr;
}

std::string EvaValue::asCppString() const
{
    if (type == EvaValueType::OBJECT && object->type == ObjectType::STRING) {
        auto sobj = (StringObject *) object;
        return sobj->string;
    } else {
        return "";
    }
}

CodeObject *EvaValue::asCodeObject() const
{
    if (type == EvaValueType::OBJECT && object->type == ObjectType::CODE) {
        return (CodeObject *) object;
    }
    return nullptr;
}
