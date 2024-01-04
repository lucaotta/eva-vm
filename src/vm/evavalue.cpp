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

FunctionObject *EvaValue::asFunction() const
{
    if (type == EvaValueType::OBJECT && object->type == ObjectType::FUNCTION) {
        return (FunctionObject *) object;
    }
    return nullptr;
}

size_t Traceable::bytesAllocated{0};
std::list<Traceable *> Traceable::objects;

void Traceable::printStats()
{
    std::cout << "Objects: " << objects.size() << "\n";
    std::cout << "Memory: " << Traceable::bytesAllocated << "\n";
    std::cout << std::endl;
}
