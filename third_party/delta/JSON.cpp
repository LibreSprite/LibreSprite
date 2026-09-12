#include "JSON.hpp"

void JSON::Value::toJSON(std::ostream& s, int depth, int inc, uint32_t encoding) {
    if (isArray()) {
        auto& arr = array();
        s << "[";
        bool first = true;
        for (auto& val : arr) {
            if (val.isFunction())
                continue;
            if (val.isUndefined() && encoding == NO_UNDEFINED)
                continue;
            if (!first) s << ",";
            first = false;
            if (inc) {
                s << "\n";
                for (int i = 0; i < depth + inc; ++i) s << " ";
            }
            val.toJSON(s, depth + inc, inc, encoding);
        }
        if (!first && inc) {
            s << "\n";
            for (int i = 0; i < depth; ++i) s << " ";
        }
        s << "]";
        return;
    }

    if (isObject()) {
        auto& obj = object();
        s << "{";
        bool first = true;
        for (auto& [key, val] : obj) {
            if (val.isFunction())
                continue;
            if (val.isUndefined() && encoding == NO_UNDEFINED)
                continue;
            if (!first) s << ",";
            first = false;
            if (inc) {
                s << "\n";
                for (int i = 0; i < depth + inc; ++i) s << " ";
            }
            s << su::escape(key);
            s << ": ";
            val.toJSON(s, depth + inc, inc, encoding);
        }
        if (!first && inc) {
            s << "\n";
            for (int i = 0; i < depth; ++i) s << " ";
        }
        s << "}";
        return;
    }

    if (isString()) {
        s << su::escape(string());
        return;
    }

    if (isNumber()) {
        s << number();
        return;
    }

    if (isBoolean()) {
        s << (boolean() ? "true" : "false");
        return;
    }

    if (isNull()) {
        s << "null";
        return;
    }

    if (isUndefined()) {
        if (encoding == NO_UNDEFINED) {
            s << "null";
        } else {
            s << "undefined";
        }
        return;
    }

    s << "ERROR";
}
