#include "evavalue.h"

double EvaValue::asNumber() const
{
    return this->number;
}

bool EvaValue::asBool() const
{
    return this->boolean;
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

NativeFunction *EvaValue::asNativeFunction() const
{
    if (type == EvaValueType::OBJECT && object->type == ObjectType::NATIVE) {
        return (NativeFunction *) object;
    }
    return nullptr;
}
