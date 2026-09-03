#pragma once

#include <stdexcept>
#include <string>
#include <vector>

/// Thrown for any malformed expression, unknown identifier, or invalid
/// mathematical operation (e.g. division by zero) encountered while
/// tokenizing / parsing / evaluating.
class ExpressionError : public std::runtime_error {
public:
    explicit ExpressionError(const std::string& message) : std::runtime_error(message) {}
};

/// Parses and evaluates a single-variable mathematical expression such as
/// "sin(x)^2 + 2x - log(x+1)" for a given value of x.
///
/// Supports:
///   - Operators: + - * / ^ and unary minus (e.g. -x, 3*-2)
///   - Implicit multiplication: 2x, 2(x+1), x(x+1), (x+1)(x-1)
///   - Functions: sin cos tan asin acos atan sinh cosh tanh sqrt exp abs log log10
///   - Constants: pi, e
///
/// This class is stateless / const-correct so a single instance can safely
/// be reused across many evaluations (e.g. once per sample point).
class ExpressionEvaluator {
public:
    double evaluate(const std::string& expression, double xValue) const;

private:
    enum class TokenType { Number, Operator, UnaryMinus, Function, Variable, LeftParen, RightParen };

    struct Token {
        std::string value;
        TokenType type;
    };

    std::vector<Token> tokenize(const std::string& expr) const;
    std::vector<Token> toPostfix(const std::vector<Token>& tokens) const;
    double evaluatePostfix(const std::vector<Token>& postfix, double xValue) const;

    static int precedence(const std::string& op);
    static bool isRightAssociative(const std::string& op);
    static bool isFunction(const std::string& token);
};
