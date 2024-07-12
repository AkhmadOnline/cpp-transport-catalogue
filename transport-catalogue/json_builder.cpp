#include "json_builder.h"
#include <stdexcept>

namespace json {

Builder::Builder() : root_(nullptr) {}

DictItemContext Builder::StartDict() {
    AddNode(Dict{});
    return DictItemContext(*this);
}

ArrayItemContext Builder::StartArray() {
    AddNode(Array{});
    return ArrayItemContext(*this);
}

Builder& Builder::EndDict() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
        throw std::logic_error("EndDict called without matching StartDict");
    }
    nodes_stack_.pop_back();
    return *this;
}

Builder& Builder::EndArray() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
        throw std::logic_error("EndArray called without matching StartArray");
    }
    nodes_stack_.pop_back();
    return *this;
}

KeyItemContext Builder::Key(std::string key) {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
        throw std::logic_error("Key method called in wrong context");
    }
    key_ = std::move(key);
    return KeyItemContext(*this);
}

Builder& Builder::Value(Node::Value value) {
    AddNode(std::visit([](auto&& val) { return Node(val); }, value));
    return *this;
}

Node Builder::Build() {
    CheckReady();
    return root_;
}

Node& Builder::AddNode(Node node) {
    Node* result;
    if (nodes_stack_.empty()) {
        if (root_.IsNull()) {
            root_ = std::move(node);
            result = &root_;
        } else {
            throw std::logic_error("Attempt to add more than one root node");
        }
    } else {
        Node& current = *nodes_stack_.back();
        if (current.IsArray()) {
            Array& arr = const_cast<Array&>(current.AsArray());
            arr.push_back(std::move(node));
            result = &arr.back();
        } else if (current.IsDict()) {
            if (!key_) {
                throw std::logic_error("Attempt to add value to dict without key");
            }
            Dict& dict = const_cast<Dict&>(current.AsDict());
            auto [it, inserted] = dict.emplace(*key_, std::move(node));
            result = &it->second;
            key_.reset();
        } else {
            throw std::logic_error("Invalid node type");
        }
    }
    if (result->IsDict() || result->IsArray()) {
        nodes_stack_.push_back(result);
    }
    return *result;
}

void Builder::CheckReady() const {
    if ((nodes_stack_.empty() && root_.IsNull()) || key_) {
        throw std::logic_error("JSON is not complete");
    }
}

KeyItemContext DictItemContext::Key(std::string key) {
    return builder_.Key(std::move(key));
}

Builder& DictItemContext::EndDict() {
    return builder_.EndDict();
}

ArrayItemContext ArrayItemContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return ArrayItemContext(builder_);
}

DictItemContext ArrayItemContext::StartDict() {
    return builder_.StartDict();
}

ArrayItemContext ArrayItemContext::StartArray() {
    return builder_.StartArray();
}

Builder& ArrayItemContext::EndArray() {
    return builder_.EndArray();
}

DictItemContext KeyItemContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return DictItemContext(builder_);
}

DictItemContext KeyItemContext::StartDict() {
    return builder_.StartDict();
}

ArrayItemContext KeyItemContext::StartArray() {
    return builder_.StartArray();
}
} // namespace json