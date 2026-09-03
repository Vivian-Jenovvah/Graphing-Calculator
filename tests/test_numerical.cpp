// Standalone correctness tests for NumericalMethods, checked against known
// closed-form answers. Dependency-free like test_evaluator.cpp:
//
//   g++ -std=c++17 -Wall -Wextra ../src/ExpressionEvaluator.cpp ../src/NumericalMethods.cpp test_numerical.cpp -o test_numerical -I..
//   ./test_numerical

#include <cmath>
#include <cstdio>
#include <string>

#include "../src/ExpressionEvaluator.h"
#include "../src/NumericalMethods.h"

namespace {

int g_failures = 0;
ExpressionEvaluator g_eval;

void expectNear(const std::string& label, double got, double expected, double tol) {
    if (std::fabs(got - expected) > tol) {
        std::printf("[FAIL] %s: got %.10g, expected %.10g (tol %.1e)\n", label.c_str(), got, expected,
                    tol);
        ++g_failures;
    } else {
        std::printf("[ OK ] %s = %.10g\n", label.c_str(), got);
    }
}

}  // namespace

int main() {
    // Derivative: d/dx[sin(x)] = cos(x)
    expectNear("d/dx[sin(x)] at x=1", numerical::derivative(g_eval, "sin(x)", 1.0), std::cos(1.0),
               1e-6);
    // d/dx[x^3] = 3x^2, at x=2 -> 12
    expectNear("d/dx[x^3] at x=2", numerical::derivative(g_eval, "x^3", 2.0), 12.0, 1e-4);

    // Integral: int_0^pi sin(x) dx = 2
    expectNear("integral sin(x) [0,pi]", numerical::simpsonIntegral(g_eval, "sin(x)", 0.0, M_PI),
               2.0, 1e-6);
    // Integral: int_0^1 x^2 dx = 1/3
    expectNear("integral x^2 [0,1]", numerical::simpsonIntegral(g_eval, "x^2", 0.0, 1.0), 1.0 / 3.0,
               1e-9);

    // Roots of sin(x) in [-1, 7]: 0, pi, 2pi
    {
        auto roots = numerical::findRoots(g_eval, "sin(x)", -1.0, 7.0);
        bool ok = roots.size() == 3 && std::fabs(roots[0] - 0.0) < 1e-5 &&
                  std::fabs(roots[1] - M_PI) < 1e-5 && std::fabs(roots[2] - 2 * M_PI) < 1e-5;
        if (ok) {
            std::printf("[ OK ] roots of sin(x) in [-1,7] = {%.6g, %.6g, %.6g}\n", roots[0], roots[1],
                        roots[2]);
        } else {
            std::printf("[FAIL] roots of sin(x) in [-1,7]: expected 3 roots near {0, pi, 2pi}, got %zu\n",
                        roots.size());
            ++g_failures;
        }
    }

    // Roots of x^2 - 4: -2, 2
    {
        auto roots = numerical::findRoots(g_eval, "x^2-4", -5.0, 5.0);
        bool ok = roots.size() == 2 && std::fabs(roots[0] + 2.0) < 1e-5 &&
                  std::fabs(roots[1] - 2.0) < 1e-5;
        if (ok) {
            std::printf("[ OK ] roots of x^2-4 in [-5,5] = {%.6g, %.6g}\n", roots[0], roots[1]);
        } else {
            std::printf("[FAIL] roots of x^2-4 in [-5,5]: expected {-2, 2}, got %zu roots\n",
                        roots.size());
            ++g_failures;
        }
    }

    // Integral should raise an error when the integrand is undefined in [a,b]
    {
        bool threw = false;
        try {
            numerical::simpsonIntegral(g_eval, "1/x", -1.0, 1.0);
        } catch (const ExpressionError&) {
            threw = true;
        }
        if (threw) {
            std::printf("[ OK ] integral of 1/x over [-1,1] correctly raised an error\n");
        } else {
            std::printf("[FAIL] integral of 1/x over [-1,1] should have raised an error\n");
            ++g_failures;
        }
    }

    if (g_failures == 0) {
        std::printf("\nAll NumericalMethods tests passed.\n");
    } else {
        std::printf("\n%d NumericalMethods test(s) FAILED.\n", g_failures);
    }
    return g_failures == 0 ? 0 : 1;
}
