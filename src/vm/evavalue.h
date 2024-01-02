#pragma once

#include <optional>
#include <string>
#include <vector>

enum class EvaValueType {
    NUMBER,
    BOOL,
    OBJECT,
    POISON,
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

struct LocalVar
{
    std::string name;
    int blockLevel{0};
};

struct CodeObject : public Object
{
    CodeObject(std::string name)
        : Object(ObjectType::CODE)
        , name(std::move(name))
    {}

    void enterBlock() { currentLevel++; }
    void exitBlock() { currentLevel--; }
    bool isGlobalScope() { return name == "main" && currentLevel == 1; }
    void addLocal(const std::string &name) { locals.push_back({name, currentLevel}); }
    std::optional<size_t> getLocalIndex(const std::string &name)
    {
        for (int i = 0; i < locals.size(); ++i) {
            if (locals[i].name == name && locals[i].blockLevel == currentLevel)
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

inline bool isPoison(const EvaValue &val)
{
    return val.type == EvaValueType::POISON;
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
    if (isPoison(value))
        return "POISON";
    return "";
}

#define NUMBER(x) EvaValue({.type = EvaValueType::NUMBER, .number = x})
#define BOOLEAN(x) EvaValue({.type = EvaValueType::BOOL, .boolean = x})
#define POISON() EvaValue({.type = EvaValueType::POISON, .boolean = false})
