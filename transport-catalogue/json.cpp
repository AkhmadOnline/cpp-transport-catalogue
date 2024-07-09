#include "json.h"

#include <cassert>

using namespace std;

namespace json {

namespace {

Node LoadNode(std::istream& input);
Node LoadArray(std::istream& input);
Node LoadDict(std::istream& input);
Node LoadString(std::istream& input);
Node LoadNull(std::istream& input);
Node LoadBool(std::istream& input);
Node LoadNumber(std::istream& input);

Node LoadArray(istream& input) {
    Array result;

    for (char c; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    if (!input) {
        throw ParsingError("Array parsing error"s);
    }

    return Node(move(result));
}

Node LoadNumber(std::istream& input) {
    std::string parsed_num;
    
    while (std::isdigit(input.peek()) || input.peek() == '-' || input.peek() == '.' || input.peek() == 'e' || input.peek() == 'E' || input.peek() == '+') {
        parsed_num.push_back(static_cast<char>(input.get()));
    }

    try {
        size_t pos = 0;
        int int_value = std::stoi(parsed_num, &pos);
        if (pos == parsed_num.length()) {
            return Node(int_value);
        }
        
        double double_value = std::stod(parsed_num);
        return Node(double_value);
    } catch (...) {
        throw ParsingError("Number parsing error"s);
    }
}

Node LoadString(istream& input) {
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            throw ParsingError("String parsing error"s);
        }
        const char ch = *it;
        if (ch == '"') {
            ++it;
            break;
        } else if (ch == '\\') {
            ++it;
            if (it == end) {
                throw ParsingError("String parsing error"s);
            }
            const char escaped_char = *(it);
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            throw ParsingError("Unexpected end of line"s);
        } else {
            s.push_back(ch);
        }
        ++it;
    }
    return Node(move(s));
}

Node LoadDict(istream& input) {
    Dict result;

    for (char c; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        std::string key = LoadString(input).AsString();
        input >> c;
        result.insert({std::move(key), LoadNode(input)});
    }

    if (!input) {
        throw ParsingError("Dict parsing error"s);
    }

    return Node(move(result));
}

Node LoadNull(std::istream& input) {
    std::string value;
    char c;
    while (input.get(c) && std::isalpha(c)) {
        value.push_back(c);
    }
    
    if (value == "null") {
        if (input.eof() || !std::isalnum(c)) {
            input.putback(c);
            return Node();
        }
    }
    
    throw ParsingError("Invalid null value: " + value);
}

Node LoadBool(std::istream& input) {
    std::string value;
    char c;
    while (input.get(c) && std::isalpha(c)) {
        value.push_back(c);
    }
    
    if (value == "true") {
        if (input.eof() || !std::isalnum(c)) {
            input.putback(c);
            return Node(true);
        }
    } else if (value == "false") {
        if (input.eof() || !std::isalnum(c)) {
            input.putback(c);
            return Node(false);
        }
    }
    
    throw ParsingError("Invalid boolean value: " + value);
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else if (c == '-' || std::isdigit(c)) {
        input.putback(c);
        return LoadNumber(input);
    } else {
        throw ParsingError("Unexpected character: "s + c);
    }
}

}  // namespace

bool Node::IsInt() const {
    return std::holds_alternative<int>(*this);  
}

bool Node::IsDouble() const {
    return std::holds_alternative<double>(*this) || IsInt();  
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(*this);  
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(*this);  
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(*this); 
}

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(*this);  
}

bool Node::IsArray() const {
    return std::holds_alternative<Array>(*this);  
}

bool Node::IsMap() const {
    return std::holds_alternative<Dict>(*this);  
}

int Node::AsInt() const {
    if (!IsInt()) {
        throw std::logic_error("Node is not an int");
    }
    return std::get<int>(*this);  
}

bool Node::AsBool() const {
    if (!IsBool()) {
        throw std::logic_error("Node is not a bool");
    }
    return std::get<bool>(*this);  
}

double Node::AsDouble() const {
    if (IsInt()) {
        return static_cast<double>(AsInt());
    }
    if (!IsPureDouble()) {
        throw std::logic_error("Node is not a double");
    }
    return std::get<double>(*this);  
}

const std::string& Node::AsString() const {
    if (!IsString()) {
        throw std::logic_error("Node is not a string");
    }
    return std::get<std::string>(*this);  
}

const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw std::logic_error("Node is not an array");
    }
    return std::get<Array>(*this);  
}

const Dict& Node::AsMap() const {
    if (!IsMap()) {
        throw std::logic_error("Node is not a map");
    }
    return std::get<Dict>(*this);  
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& other) const {
    return GetRoot() == other.GetRoot();
}

bool Document::operator!=(const Document& other) const {
    return !(*this == other);
 }

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), PrintContext{output});
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit(
        [&ctx](const auto& value) { PrintValue(value, ctx); },
        node.GetValue()
    );
}

void PrintString(const std::string& value, std::ostream& out) {
    out.put('"');
    for (const char c : value) {
        switch (c) {
            case '\r':
                out << "\\r"sv;
                break;
            case '\n':
                out << "\\n"sv;
                break;
            case '"':
                out << "\\\""sv;
                break;
            case '\\':
                out << "\\\\"sv;
                break;
            case '\t':
                out << "\\t"sv;
                break;
            default:
                out.put(c);
        }
    }
    out.put('"');
}

template <typename Value>
void PrintValue(const Value& value, const PrintContext& ctx) {
    ctx.out << value;
}

void PrintValue(std::nullptr_t, const PrintContext& ctx) {
    ctx.out << "null"sv;
}

void PrintValue(const std::string& value, const PrintContext& ctx) {
    PrintString(value, ctx.out);
}

void PrintValue(bool value, const PrintContext& ctx) {
    ctx.out << (value ? "true"sv : "false"sv);
}

void PrintValue(const Array& nodes, const PrintContext& ctx) {
    std::ostream& out = ctx.out;
    out << "[\n"sv;
    bool first = true;
    auto inner_ctx = ctx.Indented();
    for (const Node& node : nodes) {
        if (first) {
            first = false;
        } else {
            out << ",\n"sv;
        }
        inner_ctx.PrintIndent();
        PrintNode(node, inner_ctx);
    }
    out << '\n';
    ctx.PrintIndent();
    out << "]"sv;
}

void PrintValue(const Dict& nodes, const PrintContext& ctx) {
    std::ostream& out = ctx.out;
    out << "{\n"sv;
    bool first = true;
    auto inner_ctx = ctx.Indented();
    for (const auto& [key, node] : nodes) {
        if (first) {
            first = false;
        } else {
            out << ",\n"sv;
        }
        inner_ctx.PrintIndent();
        PrintString(key, ctx.out);
        out << ": "sv;
        PrintNode(node, inner_ctx);
    }
    out << '\n';
    ctx.PrintIndent();
    out << "}"sv;
}

}  // namespace json