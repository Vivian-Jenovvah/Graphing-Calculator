#pragma once

#include <string>
#include <vector>

#include "ExpressionEvaluator.h"
#include "GraphData.h"
#include "imgui.h"

/// One user-entered function and everything derived from it: its sampled
/// points, its optional numerical-derivative overlay, and its optional
/// root markers. Each curve is plotted in its own color so multiple curves
/// can be compared on one graph.
struct Curve {
    char exprBuf[256] = "";
    ImVec4 color{1, 1, 1, 1};
    bool enabled = true;
    bool showDerivative = false;
    bool showRoots = false;

    GraphData data;        // f(x)
    GraphData derivative;  // f'(x), only populated when showDerivative is set
    std::vector<double> roots;
    std::string error;
};

/// Settings + result for the "definite integral" panel, which operates on
/// one selected curve at a time.
struct IntegralPanel {
    int curveIndex = 0;
    float a = -1.0f;
    float b = 1.0f;
    bool showShading = false;
    bool hasResult = false;
    double result = 0.0;
    std::string error;

    // Shaded region sample points, cached so renderPlot() doesn't need to
    // re-evaluate the expression every frame. Shaded down to y = 0.
    std::vector<double> shadeX, shadeY;
};

/// Owns all calculator state (multiple curves, integral panel) and draws
/// the whole UI each frame via Dear ImGui / ImPlot. Call Render() once per
/// frame from the main loop.
class CalculatorApp {
public:
    CalculatorApp();

    void Render();

private:
    void renderCurveList();
    void renderRangeControls();
    void renderButtonPad();
    void renderIntegralPanel();
    void renderPlot();

    void addCurve();
    void removeCurve(int index);
    void plotAll();
    void plotCurve(Curve& curve);
    void computeIntegral();
    void insertText(const std::string& text);

    ExpressionEvaluator evaluator_;
    std::vector<Curve> curves_;
    int activeCurveIndex_ = 0;  // which curve the on-screen keypad types into

    IntegralPanel integral_;

    float xMin_ = -10.0f;
    float xMax_ = 10.0f;
    int samples_ = 600;

    std::string globalError_;
};
