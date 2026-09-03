#pragma once

#include <vector>

/// Holds the (x, y) sample points for one plotted curve.
/// A NaN y-value is used deliberately to mark a discontinuity (e.g. an
/// asymptote or an out-of-domain point) so the renderer can break the line
/// there instead of drawing a spurious vertical segment.
class GraphData {
public:
    void clear();
    void addPoint(double x, double y);

    const std::vector<double>& xValues() const { return x_values_; }
    const std::vector<double>& yValues() const { return y_values_; }
    size_t size() const { return x_values_.size(); }

private:
    std::vector<double> x_values_;
    std::vector<double> y_values_;
};
