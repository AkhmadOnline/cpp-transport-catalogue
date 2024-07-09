#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
// Сохраните объявления Dict и Array без изменения
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
   /* Реализуйте Node, используя std::variant */
    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    Node();
    Node(Array array);
    Node(Dict map);
    Node(int value);
    Node(double value);
    Node(std::string value);
    Node(bool value);
    Node(std::nullptr_t);

    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    const Value& GetValue() const { return value_; }

    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;

private:
    Value value_;
};

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator==(const Document& other) const;

    bool operator!=(const Document& other) const;
private:
    Node root_;
};

Document Load(std::istream& input);

struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

void Print(const Document& doc, std::ostream& output);

void PrintNode(const Node& node, const PrintContext& ctx);

void PrintString(const std::string& value, std::ostream& out);

template <typename Value>
void PrintValue(const Value& value, const PrintContext& ctx);

void PrintValue(std::nullptr_t, const PrintContext& ctx);
void PrintValue(const std::string& value, const PrintContext& ctx);
void PrintValue(bool value, const PrintContext& ctx);
void PrintValue(const Array& nodes, const PrintContext& ctx);
void PrintValue(const Dict& nodes, const PrintContext& ctx);

}  // namespace json