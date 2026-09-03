// Standalone correctness tests for ExpressionEvaluator.
//
// Deliberately dependency-free (no GTest/Catch2) so it can be compiled and
// run with a plain compiler invocation, independent of the GLFW/ImGui/ImPlot
// GUI stack:
//
//   g++ -std=c++17 -Wall -Wextra ../src/ExpressionEvaluator.cpp test_evaluator.cpp -o test_evaluator -I..
//   ./test_evaluator
//
// Exits non-zero (and prints which case failed) if any check fails, so it
// can also be wired into a CI job.

#include <cmath>
#include <cstdio>
#include <string>

#include "../src/ExpressionEvaluator.h"

namespace {

int g_failures = 0;

void expectNear(const std::string& expr, double x, double expected, double tol = 1e-6) {
    ExpressionEvaluator eval;
    try {
        double got = eval.evaluate(expr, x);
        if (std::fabs(got - expected) > tol) {
            std::printf("[FAIL] %s at x=%g: got %.10g, expected %.10g\n", expr.c_str(), x, got,
                        expected);
            ++g_failures;
        } else {
            std::printf("[ OK ] %s at x=%g = %.10g\n", expr.c_str(), x, got);
        }
    } catch (const ExpressionError& e) {
        std::printf("[FAIL] %s at x=%g threw unexpectedly: %s\n", expr.c_str(), x, e.what());
        ++g_failures;
    }
}

void expectError(const std::string& expr, double x = 0.0) {
    ExpressionEvaluator eval;
    try {
        double got = eval.evaluate(expr, x);
        std::printf("[FAIL] %s at x=%g: expected an error, got %.10g\n", expr.c_str(), x, got);
        ++g_failures;
    } catch (const ExpressionError&) {
        std::printf("[ OK ] %s at x=%g correctly raised an error\n", expr.c_str(), x);
    }
}

}  // namespace

int main() {
    // Basic arithmetic and precedence
    expectNear("2+3*4", 0, 14);
    expectNear("(2+3)*4", 0, 20);
    expectNear("2^3^2", 0, 512);  // right-associative: 2^(3^2) = 2^9

    // Unary minus
    expectNear("-x", 5, -5);
    expectNear("3*-2", 0, -6);
    expectNear("-(2+3)", 0, -5);

    // Implicit multiplication
    expectNear("2x", 3, 6);
    expectNear("2(x+1)", 4, 10);
    expectNear("(x+1)(x-1)", 3, 8);
    expectNear("2pi", 0, 2 * M_PI, 1e-5);

    // Functions and constants
    expectNear("sin(x)^2 + cos(x)^2", 1.234, 1.0);
    expectNear("sqrt(x^2)", -4, 4);
    expectNear("log(e)", 0, 1.0, 1e-5);
    expectNear("log10(100)", 0, 2.0);
    expectNear("abs(-7)", 0, 7);

    // Error handling
    expectError("1/(x-x)", 5);   // division by zero
    expectError("sqrt(-4)");     // negative sqrt
    expectError("((1+2)");       // mismatched parens
    expectError("2++");          // malformed
    expectError("foo(x)");       // unknown identifier

    if (g_failures == 0) {
        std::printf("\nAll ExpressionEvaluator tests passed.\n");
    } else {
        std::printf("\n%d ExpressionEvaluator test(s) FAILED.\n", g_failures);
    }
    return g_failures == 0 ? 0 : 1;
}
