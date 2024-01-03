#pragma once

#include <functional>
#include <optional>
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
    NATIVE,
    FUNCTION,
};

using NativeFn = std::function<void()>;

struct Object;
struct StringObject;
struct CodeObject;
struct NativeFunction;
struct FunctionObject;

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
    NativeFunction *asNativeFunction() const;
    FunctionObject *asFunction() const;
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

struct LocalVar
{
    std::string name;
    int blockLevel{0};
};

struct CodeObject : public Object
{
    CodeObject(std::string name, int arity)
        : Object(ObjectType::CODE)
        , name(std::move(name))
        , arity(arity)
    {}

    void enterBlock() { currentLevel++; }
    void exitBlock() { currentLevel--; }
    bool isGlobalScope() { return name == "main" && currentLevel == 1; }
    void addLocal(const std::string &name) { locals.push_back({name, currentLevel}); }
    void addConst(EvaValue val) { constants.push_back(val); }
    std::optional<size_t> getLocalIndex(const std::string &name)
    {
        // Start from the end, which are the latest defined locals.
        // Accept all variable names that have been defined in outer blocks
        for (int i = locals.size(); i > 0; --i) {
            if (locals[i].name == name && locals[i].blockLevel <= currentLevel)
                return i;
        }
        return {};
    }
    size_t variableNumberInCurrentBlock()
    {
        size_t count{0};
        if (locals.size() > 0) {
            while (locals.back().blockLevel == currentLevel) {
                locals.pop_back();
                count++;
            }
        }
        return count;
    }

    std::string name;
    std::vector<uint8_t> code;
    std::vector<EvaValue> constants;
    int currentLevel{0};
    std::vector<LocalVar> locals;
    int arity{0};
};

struct NativeFunction : public Object
{
    NativeFunction(NativeFn fn, std::string name, int arity)
        : Object(ObjectType::NATIVE)
        , fn(std::move(fn))
        , name(std::move(name))
        , arity(arity)
    {}
    NativeFn fn;
    std::string name;
    int arity{0};
};

struct FunctionObject : public Object
{
    FunctionObject(CodeObject *co)
        : Object(ObjectType::FUNCTION)
        , co(co)
    {}
    CodeObject *co;
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

inline bool isNative(const EvaValue &val)
{
    return isObjectType(val, ObjectType::NATIVE);
}

inline bool isFunction(const EvaValue &val)
{
    return isObjectType(val, ObjectType::FUNCTION);
}

inline EvaValue allocString(std::string str)
{
    return EvaValue{.type = EvaValueType::OBJECT, .object = new StringObject(std::move(str))};
}

inline EvaValue allocCode(std::string name, int arity)
{
    return EvaValue{.type = EvaValueType::OBJECT, .object = new CodeObject(std::move(name), arity)};
}

inline EvaValue allocNative(std::function<void()> fn, std::string name, int arity)
{
    return EvaValue{.type = EvaValueType::OBJECT, .object = new NativeFunction(fn, name, arity)};
}

inline EvaValue allocFunction(CodeObject *co)
{
    return EvaValue{.type = EvaValueType::OBJECT, .object = new FunctionObject(co)};
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
    if (isNative(value)) {
        return "NATIVE " + value.asNativeFunction()->name + "/"
               + std::to_string(value.asNativeFunction()->arity);
    }
    if (isFunction(value)) {
        return "FUNCTION " + value.asFunction()->co->name + "/"
               + std::to_string(value.asFunction()->co->arity);
    }
    return "";
}

#define NUMBER(x) EvaValue({.type = EvaValueType::NUMBER, .number = x})
#define BOOLEAN(x) EvaValue({.type = EvaValueType::BOOL, .boolean = x})
