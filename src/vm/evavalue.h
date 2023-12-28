#pragma once

enum class EvaValueType {
    NUMBER,
};

struct EvaValue {
    EvaValueType type;
    union {
        double number;
    };

    double asNumber() { return this->number; }
};

#define NUMBER(x) EvaValue({EvaValueType::NUMBER, .number = x})
