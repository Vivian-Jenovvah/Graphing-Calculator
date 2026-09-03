#include "GraphData.h"

void GraphData::clear() {
    x_values_.clear();
    y_values_.clear();
}

void GraphData::addPoint(double x, double y) {
    x_values_.push_back(x);
    y_values_.push_back(y);
}
