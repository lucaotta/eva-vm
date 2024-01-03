#pragma once

#include "evavalue.h"

#include <algorithm>
#include <optional>
#include <string>

class Globals
{
public:
    EvaValue get(size_t index)
    {
        if (index < m_values.size()) {
            return m_values[index].value;
        }
        return {};
    }

    std::string nameForIndex(size_t index)
    {
        if (index < m_values.size()) {
            return m_values[index].name;
        }
        return "";
    }

    void set(size_t index, EvaValue v)
    {
        if (index < m_values.size()) {
            m_values[index].value = v;
        }
    }

    std::optional<size_t> getGlobalIndex(const std::string &name)
    {
        for (int i = 0; i < m_values.size(); ++i) {
            if (m_values[i].name == name)
                return i;
        }
        return {};
    }
    std::optional<size_t> define(const std::string &name)
    {
        if (!exists(name)) {
            m_values.push_back({name, NUMBER(0)});
            return m_values.size() - 1;
        }
        return {};
    }

    bool exists(const std::string &name) { return find(name) != m_values.end(); }

    void addConst(const std::string &name, EvaValue v)
    {
        if (exists(name))
            return;
        m_values.push_back({name, v});
    }

    void addNativeFunction(const std::string &name, std::function<void()> fn, int arity)
    {
        if (exists(name))
            return;
        m_values.push_back({name, allocNative(fn, name, arity)});
    }

private:
    struct Variable
    {
        std::string name;
        EvaValue value;
    };

    std::vector<Variable>::iterator find(const std::string &name)
    {
        return std::find_if(m_values.begin(), m_values.end(), [&name](const Variable &v) {
            return v.name == name;
        });
    }

    std::vector<Variable> m_values;
};
