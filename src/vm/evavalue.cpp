#include "evavalue.h"

double EvaValue::asNumber() { return this->number; }

Object *EvaValue::asObject() { return this->object; }

StringObject *EvaValue::asString()
{
    if (type == EvaValueType::OBJECT && object->type == ObjectType::STRING) {
        return (StringObject *) object;
    }
    return nullptr;
}

std::string EvaValue::asCppString()
{
    if (type == EvaValueType::OBJECT && object->type == ObjectType::STRING) {
        auto sobj = (StringObject *) object;
        return sobj->string;
    } else {
        return "";
    }
}
