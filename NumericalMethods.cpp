#include "NumericalMethods.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace numerical {

namespace {

bool tryEvaluate(const ExpressionEvaluator& eval, const std::string& expression, double x,
                  double& outY) {
    try {
        outY = eval.evaluate(expression, x);
    } catch (const ExpressionError&) {
        return false;
    }
    return std::isfinite(outY);
}

}  // namespace

double derivative(const ExpressionEvaluator& eval, const std::string& expression, double x,
                   double h) {
    double fPlus, fMinus;
    if (!tryEvaluate(eval, expression, x + h, fPlus) || !tryEvaluate(eval, expression, x - h, fMinus)) {
        throw ExpressionError("Derivative is undefined at x = " + std::to_string(x) + ".");
    }
    return (fPlus - fMinus) / (2.0 * h);
}

double simpsonIntegral(const ExpressionEvaluator& eval, const std::string& expression, double a,
                        double b, int segments) {
    if (segments < 2) segments = 2;
    if (segments % 2 != 0) segments += 1;

    if (b < a) std::swap(a, b);  // Simpson's rule assumes a < b

    const double h = (b - a) / segments;

    double f0, fN;
    if (!tryEvaluate(eval, expression, a, f0) || !tryEvaluate(eval, expression, b, fN)) {
        throw ExpressionError("Integrand is undefined at the bounds of [a, b].");
    }

    double sum = f0 + fN;
    for (int i = 1; i < segments; ++i) {
        double x = a + i * h;
        double fx;
        if (!tryEvaluate(eval, expression, x, fx)) {
            throw ExpressionError("Integrand is undefined at x = " + std::to_string(x) +
                                   " within [a, b].");
        }
        sum += (i % 2 == 0 ? 2.0 : 4.0) * fx;
    }

    return sum * h / 3.0;
}

std::vector<double> findRoots(const ExpressionEvaluator& eval, const std::string& expression,
                               double xMin, double xMax, int samples, double tolerance) {
    std::vector<double> roots;
    if (samples < 2 || xMax <= xMin) return roots;

    const double step = (xMax - xMin) / samples;

    double prevX = xMin;
    double prevY;
    bool prevValid = tryEvaluate(eval, expression, prevX, prevY);

    for (int i = 1; i <= samples; ++i) {
        double x = xMin + i * step;
        double y;
        bool valid = tryEvaluate(eval, expression, x, y);

        if (prevValid && valid) {
            if (prevY == 0.0) {
                roots.push_back(prevX);
            } else if ((prevY < 0.0) != (y < 0.0)) {
                // Bracket found -- refine via bisection.
                double lo = prevX, hi = x, flo = prevY;
                double root = 0.5 * (lo + hi);
                for (int iter = 0; iter < 100; ++iter) {
                    double mid = 0.5 * (lo + hi);
                    double fmid;
                    if (!tryEvaluate(eval, expression, mid, fmid)) break;
                    root = mid;
                    if (std::fabs(fmid) < tolerance || (hi - lo) < tolerance) break;
                    if ((flo < 0.0) == (fmid < 0.0)) {
                        lo = mid;
                        flo = fmid;
                    } else {
                        hi = mid;
                    }
                }
                roots.push_back(root);
            }
        }

        prevX = x;
        prevY = y;
        prevValid = valid;
    }

    // Adjacent sample brackets can independently converge to essentially the
    // same root (e.g. a zero that lands exactly on a sample point gets
    // caught by both the "prevY == 0" check and the bracket on either side
    // of it) -- merge anything closer together than 10x the tolerance.
    if (!roots.empty()) {
        std::sort(roots.begin(), roots.end());
        std::vector<double> deduped;
        deduped.reserve(roots.size());
        deduped.push_back(roots.front());
        for (size_t i = 1; i < roots.size(); ++i) {
            if (roots[i] - deduped.back() > tolerance * 10.0) {
                deduped.push_back(roots[i]);
            }
        }
        roots = std::move(deduped);
    }

    return roots;
}

}  // namespace numerical
