#include "ExpressionEvaluator.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stack>

namespace {

const std::vector<std::string>& knownFunctions() {
    static const std::vector<std::string> funcs = {
        "sin", "cos", "tan", "asin", "acos", "atan",
        "sinh", "cosh", "tanh", "sqrt", "exp", "abs", "log10", "log"
    };
    return funcs;
}

} // namespace

bool ExpressionEvaluator::isFunction(const std::string& token) {
    const auto& funcs = knownFunctions();
    return std::find(funcs.begin(), funcs.end(), token) != funcs.end();
}

int ExpressionEvaluator::precedence(const std::string& op) {
    if (op == "+" || op == "-") return 1;
    if (op == "*" || op == "/") return 2;
    if (op == "u-") return 3;  // unary minus binds tighter than * /
    if (op == "^") return 4;
    return 0;
}

bool ExpressionEvaluator::isRightAssociative(const std::string& op) {
    return op == "^" || op == "u-";
}

std::vector<ExpressionEvaluator::Token> ExpressionEvaluator::tokenize(const std::string& expr) const {
    std::vector<Token> tokens;
    size_t i = 0;

    auto prevSignalsUnary = [&tokens]() {
        if (tokens.empty()) return true;
        TokenType t = tokens.back().type;
        return t == TokenType::Operator || t == TokenType::UnaryMinus || t == TokenType::LeftParen;
    };

    while (i < expr.length()) {
        char c = expr[i];

        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            std::string number;
            while (i < expr.length() &&
                   (std::isdigit(static_cast<unsigned char>(expr[i])) || expr[i] == '.')) {
                number += expr[i++];
            }
            // scientific notation, e.g. 1e-5, 2.3E10
            if (i < expr.length() && (expr[i] == 'e' || expr[i] == 'E') &&
                (i + 1 < expr.length()) &&
                (std::isdigit(static_cast<unsigned char>(expr[i + 1])) ||
                 ((expr[i + 1] == '+' || expr[i + 1] == '-') && i + 2 < expr.length() &&
                  std::isdigit(static_cast<unsigned char>(expr[i + 2]))))) {
                number += expr[i++];
                if (expr[i] == '+' || expr[i] == '-') number += expr[i++];
                while (i < expr.length() && std::isdigit(static_cast<unsigned char>(expr[i])))
                    number += expr[i++];
            }
            tokens.push_back({number, TokenType::Number});
        } else if (std::isalpha(static_cast<unsigned char>(c))) {
            // Identifiers are a letter followed by letters/digits, e.g. "sin",
            // "x", "log10" -- the trailing-digit case matters so "log10" is
            // read as one identifier and not split into "log" + the number 10.
            std::string ident;
            while (i < expr.length() && (std::isalpha(static_cast<unsigned char>(expr[i])) ||
                                          std::isdigit(static_cast<unsigned char>(expr[i]))))
                ident += expr[i++];

            if (ident == "x") {
                tokens.push_back({ident, TokenType::Variable});
            } else if (ident == "pi") {
                tokens.push_back({std::to_string(M_PI), TokenType::Number});
            } else if (ident == "e") {
                tokens.push_back({std::to_string(M_E), TokenType::Number});
            } else if (isFunction(ident)) {
                tokens.push_back({ident, TokenType::Function});
            } else {
                throw ExpressionError("Unknown identifier '" + ident + "'.");
            }
        } else if (c == '-' && prevSignalsUnary()) {
            tokens.push_back({"u-", TokenType::UnaryMinus});
            i++;
        } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') {
            tokens.push_back({std::string(1, c), TokenType::Operator});
            i++;
        } else if (c == '(') {
            tokens.push_back({"(", TokenType::LeftParen});
            i++;
        } else if (c == ')') {
            tokens.push_back({")", TokenType::RightParen});
            i++;
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            i++;
        } else {
            throw ExpressionError(std::string("Invalid character '") + c + "' in expression.");
        }
    }

    // Second pass: insert implicit multiplication, e.g. 2x -> 2*x, 2(x) -> 2*(x),
    // x(x+1) -> x*(x+1), (x+1)(x-1) -> (x+1)*(x-1), 2pi -> handled since pi becomes Number already.
    std::vector<Token> withImplicitMul;
    withImplicitMul.reserve(tokens.size());
    for (size_t k = 0; k < tokens.size(); ++k) {
        if (k > 0) {
            const Token& prev = tokens[k - 1];
            const Token& cur = tokens[k];
            bool prevEndsValue = prev.type == TokenType::Number || prev.type == TokenType::Variable ||
                                  prev.type == TokenType::RightParen;
            bool curStartsValue = cur.type == TokenType::Number || cur.type == TokenType::Variable ||
                                   cur.type == TokenType::Function || cur.type == TokenType::LeftParen;
            if (prevEndsValue && curStartsValue) {
                withImplicitMul.push_back({"*", TokenType::Operator});
            }
        }
        withImplicitMul.push_back(tokens[k]);
    }

    if (withImplicitMul.empty()) throw ExpressionError("Expression is empty.");

    return withImplicitMul;
}

std::vector<ExpressionEvaluator::Token> ExpressionEvaluator::toPostfix(
    const std::vector<Token>& tokens) const {
    std::vector<Token> output;
    std::stack<Token> opStack;

    for (const auto& token : tokens) {
        switch (token.type) {
            case TokenType::Number:
            case TokenType::Variable:
                output.push_back(token);
                break;

            case TokenType::Function:
            case TokenType::UnaryMinus:
                opStack.push(token);
                break;

            case TokenType::Operator:
                while (!opStack.empty() &&
                       (opStack.top().type == TokenType::Function ||
                        opStack.top().type == TokenType::UnaryMinus ||
                        (opStack.top().type == TokenType::Operator &&
                         (precedence(opStack.top().value) > precedence(token.value) ||
                          (precedence(opStack.top().value) == precedence(token.value) &&
                           !isRightAssociative(token.value)))))) {
                    output.push_back(opStack.top());
                    opStack.pop();
                }
                opStack.push(token);
                break;

            case TokenType::LeftParen:
                opStack.push(token);
                break;

            case TokenType::RightParen: {
                bool matched = false;
                while (!opStack.empty() && opStack.top().type != TokenType::LeftParen) {
                    output.push_back(opStack.top());
                    opStack.pop();
                }
                if (!opStack.empty()) {
                    opStack.pop();  // discard '('
                    matched = true;
                }
                if (!matched) throw ExpressionError("Mismatched parentheses.");
                if (!opStack.empty() && opStack.top().type == TokenType::Function) {
                    output.push_back(opStack.top());
                    opStack.pop();
                }
                break;
            }
        }
    }

    while (!opStack.empty()) {
        if (opStack.top().type == TokenType::LeftParen) throw ExpressionError("Mismatched parentheses.");
        output.push_back(opStack.top());
        opStack.pop();
    }

    return output;
}

double ExpressionEvaluator::evaluatePostfix(const std::vector<Token>& postfix, double xValue) const {
    std::stack<double> values;

    auto pop1 = [&values]() -> double {
        if (values.empty()) throw ExpressionError("Malformed expression.");
        double v = values.top();
        values.pop();
        return v;
    };

    for (const auto& token : postfix) {
        if (token.type == TokenType::Number) {
            values.push(std::stod(token.value));
        } else if (token.type == TokenType::Variable) {
            values.push(xValue);
        } else if (token.type == TokenType::UnaryMinus) {
            values.push(-pop1());
        } else if (token.type == TokenType::Operator) {
            double b = pop1();
            double a = pop1();
            if (token.value == "+") values.push(a + b);
            else if (token.value == "-") values.push(a - b);
            else if (token.value == "*") values.push(a * b);
            else if (token.value == "/") {
                if (b == 0.0) throw ExpressionError("Division by zero.");
                values.push(a / b);
            } else if (token.value == "^") {
                values.push(std::pow(a, b));
            }
        } else if (token.type == TokenType::Function) {
            double arg = pop1();
            if (token.value == "sin") values.push(std::sin(arg));
            else if (token.value == "cos") values.push(std::cos(arg));
            else if (token.value == "tan") values.push(std::tan(arg));
            else if (token.value == "asin") values.push(std::asin(arg));
            else if (token.value == "acos") values.push(std::acos(arg));
            else if (token.value == "atan") values.push(std::atan(arg));
            else if (token.value == "sinh") values.push(std::sinh(arg));
            else if (token.value == "cosh") values.push(std::cosh(arg));
            else if (token.value == "tanh") values.push(std::tanh(arg));
            else if (token.value == "sqrt") {
                if (arg < 0.0) throw ExpressionError("sqrt of a negative number.");
                values.push(std::sqrt(arg));
            } else if (token.value == "exp") values.push(std::exp(arg));
            else if (token.value == "abs") values.push(std::fabs(arg));
            else if (token.value == "log10") values.push(std::log10(arg));
            else if (token.value == "log") values.push(std::log(arg));
        }
    }

    if (values.size() != 1) throw ExpressionError("Malformed expression.");
    return values.top();
}

double ExpressionEvaluator::evaluate(const std::string& expression, double xValue) const {
    if (expression.empty()) throw ExpressionError("Expression is empty.");
    auto tokens = tokenize(expression);
    auto postfix = toPostfix(tokens);
    return evaluatePostfix(postfix, xValue);
}
