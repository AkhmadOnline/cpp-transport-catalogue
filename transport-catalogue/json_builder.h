#pragma once

#include "json.h"
#include <string>
#include <vector>
#include <optional>

namespace json {

class Builder;
class DictItemContext;
class ArrayItemContext;
class KeyItemContext;

class BaseContext {
public:
    BaseContext(Builder& builder) : builder_(builder) {}

protected:
    Builder& builder_;
};

class Builder {
public:
    Builder();

    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& EndDict();
    Builder& EndArray();
    KeyItemContext Key(std::string key);
    Builder& Value(Node::Value value);
    Node Build();

private:
    Node root_;
    std::vector<Node*> nodes_stack_;
    std::optional<std::string> key_;

    Node& AddNode(Node node);
    void CheckReady() const;
};

class DictItemContext : public BaseContext {
public:
    using BaseContext::BaseContext;
    KeyItemContext Key(std::string key);
    Builder& EndDict();
};

class ArrayItemContext : public BaseContext {
public:
    using BaseContext::BaseContext;
    ArrayItemContext Value(Node::Value value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& EndArray();
};

class KeyItemContext : public BaseContext {
public:
    using BaseContext::BaseContext;
    DictItemContext Value(Node::Value value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
};

} // namespace json