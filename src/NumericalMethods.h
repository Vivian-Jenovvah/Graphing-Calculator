#pragma once

#include <string>
#include <vector>

#include "ExpressionEvaluator.h"

/// Small, independently testable numerical-methods module used by the
/// calculator's "Tier 1" features: derivative overlay, definite integral,
/// and root finding. Each function takes the shared ExpressionEvaluator
/// instance plus the expression string, mirroring ExpressionEvaluator's own
/// (expression, x) calling convention so callers don't need to re-parse.
namespace numerical {

/// Approximates f'(x) using the central difference formula
/// (f(x+h) - f(x-h)) / 2h. Throws ExpressionError if the expression can't
/// be evaluated at x+h or x-h.
double derivative(const ExpressionEvaluator& eval, const std::string& expression, double x,
                   double h = 1e-4);

/// Approximates the definite integral of `expression` over [a, b] using
/// composite Simpson's rule with the given number of segments (rounded up
/// to the nearest even number, minimum 2). Throws ExpressionError if the
/// expression is undefined anywhere in [a, b] or produces a non-finite
/// result.
double simpsonIntegral(const ExpressionEvaluator& eval, const std::string& expression, double a,
                        double b, int segments = 1000);

/// Scans `expression` over [xMin, xMax] at `samples` evenly spaced points
/// looking for sign changes, then refines each bracket to `tolerance` via
/// bisection. Points where the expression is undefined (non-finite) are
/// skipped rather than treated as a sign change. Returns the roots found,
/// in ascending order.
std::vector<double> findRoots(const ExpressionEvaluator& eval, const std::string& expression,
                               double xMin, double xMax, int samples = 2000,
                               double tolerance = 1e-7);

}  // namespace numerical
