#include "CalculatorApp.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "implot.h"

#include "NumericalMethods.h"

namespace {
constexpr int kMaxSamples = 5000;
constexpr int kMinSamples = 10;
constexpr int kMaxCurves = 8;

// A small, visually distinct color cycle (same palette family matplotlib
// uses) so curves stay easy to tell apart.
constexpr ImVec4 kPalette[] = {
    {0.12f, 0.47f, 0.71f, 1.0f}, {1.00f, 0.50f, 0.05f, 1.0f}, {0.17f, 0.63f, 0.17f, 1.0f},
    {0.84f, 0.15f, 0.16f, 1.0f}, {0.58f, 0.40f, 0.74f, 1.0f}, {0.09f, 0.75f, 0.81f, 1.0f},
    {0.89f, 0.47f, 0.76f, 1.0f}, {0.74f, 0.74f, 0.13f, 1.0f},
};

std::string curveLabel(const Curve& curve, int index) {
    std::string expr(curve.exprBuf);
    if (expr.empty()) return "curve " + std::to_string(index + 1);
    return expr;
}
}  // namespace

CalculatorApp::CalculatorApp() {
    addCurve();
    std::strncpy(curves_[0].exprBuf, "sin(x) + 0.5*x", sizeof(curves_[0].exprBuf) - 1);
    plotAll();
}

void CalculatorApp::addCurve() {
    if (static_cast<int>(curves_.size()) >= kMaxCurves) return;
    Curve c;
    c.color = kPalette[curves_.size() % (sizeof(kPalette) / sizeof(kPalette[0]))];
    curves_.push_back(c);
    activeCurveIndex_ = static_cast<int>(curves_.size()) - 1;
}

void CalculatorApp::removeCurve(int index) {
    if (index < 0 || index >= static_cast<int>(curves_.size())) return;
    curves_.erase(curves_.begin() + index);
    if (curves_.empty()) addCurve();
    activeCurveIndex_ = std::clamp(activeCurveIndex_, 0, static_cast<int>(curves_.size()) - 1);
    integral_.curveIndex = std::clamp(integral_.curveIndex, 0, static_cast<int>(curves_.size()) - 1);
}

void CalculatorApp::insertText(const std::string& text) {
    if (activeCurveIndex_ < 0 || activeCurveIndex_ >= static_cast<int>(curves_.size())) return;
    Curve& curve = curves_[activeCurveIndex_];
    std::string current(curve.exprBuf);
    if (current.size() + text.size() < sizeof(curve.exprBuf) - 1) {
        current += text;
        std::strncpy(curve.exprBuf, current.c_str(), sizeof(curve.exprBuf) - 1);
        curve.exprBuf[sizeof(curve.exprBuf) - 1] = '\0';
    }
}

void CalculatorApp::plotCurve(Curve& curve) {
    curve.error.clear();
    curve.data.clear();
    curve.derivative.clear();
    curve.roots.clear();

    std::string expr(curve.exprBuf);
    if (expr.empty()) {
        curve.error = "Empty expression.";
        return;
    }

    const int n = std::clamp(samples_, kMinSamples, kMaxSamples);
    const double step = (static_cast<double>(xMax_) - static_cast<double>(xMin_)) / (n - 1);

    bool anyValid = false;
    try {
        for (int i = 0; i < n; ++i) {
            double x = static_cast<double>(xMin_) + i * step;
            double y = evaluator_.evaluate(expr, x);
            if (!std::isfinite(y)) {
                curve.data.addPoint(x, std::nan(""));  // break the line at asymptotes
            } else {
                curve.data.addPoint(x, y);
                anyValid = true;
            }

            if (curve.showDerivative) {
                try {
                    double dy = numerical::derivative(evaluator_, expr, x);
                    curve.derivative.addPoint(x, std::isfinite(dy) ? dy : std::nan(""));
                } catch (const ExpressionError&) {
                    curve.derivative.addPoint(x, std::nan(""));
                }
            }
        }
    } catch (const ExpressionError& e) {
        curve.error = e.what();
        curve.data.clear();
        curve.derivative.clear();
        return;
    }

    if (!anyValid) {
        curve.error = "No valid (finite) points in this range.";
        return;
    }

    if (curve.showRoots) {
        try {
            curve.roots = numerical::findRoots(evaluator_, expr, xMin_, xMax_);
        } catch (const ExpressionError&) {
            // Root finding is best-effort; leave roots empty on failure
            // without blocking the rest of the plot.
        }
    }
}

void CalculatorApp::plotAll() {
    globalError_.clear();
    if (xMax_ <= xMin_) {
        globalError_ = "x max must be greater than x min.";
        return;
    }
    for (auto& curve : curves_) {
        if (curve.enabled) plotCurve(curve);
    }
}

void CalculatorApp::computeIntegral() {
    integral_.error.clear();
    integral_.hasResult = false;
    integral_.shadeX.clear();
    integral_.shadeY.clear();

    if (curves_.empty()) {
        integral_.error = "Add a curve first.";
        return;
    }
    integral_.curveIndex = std::clamp(integral_.curveIndex, 0, static_cast<int>(curves_.size()) - 1);
    std::string expr(curves_[integral_.curveIndex].exprBuf);
    if (expr.empty()) {
        integral_.error = "Selected curve has no expression.";
        return;
    }
    if (integral_.a == integral_.b) {
        integral_.error = "a and b must differ.";
        return;
    }

    try {
        integral_.result =
            numerical::simpsonIntegral(evaluator_, expr, integral_.a, integral_.b);
        integral_.hasResult = true;

        double lo = std::min(integral_.a, integral_.b);
        double hi = std::max(integral_.a, integral_.b);
        constexpr int n = 200;
        for (int i = 0; i < n; ++i) {
            double x = lo + i * (hi - lo) / (n - 1);
            try {
                double y = evaluator_.evaluate(expr, x);
                if (std::isfinite(y)) {
                    integral_.shadeX.push_back(x);
                    integral_.shadeY.push_back(y);
                }
            } catch (const ExpressionError&) {
                // Skip points where the shading preview can't be evaluated;
                // the integral result itself already reflects any error.
            }
        }
    } catch (const ExpressionError& e) {
        integral_.error = e.what();
    }
}

void CalculatorApp::renderCurveList() {
    ImGui::Text("Curves");
    ImGui::SameLine();
    ImGui::TextDisabled("(click a field to make it active for the keypad)");

    int pendingRemoval = -1;

    for (int i = 0; i < static_cast<int>(curves_.size()); ++i) {
        Curve& curve = curves_[i];
        ImGui::PushID(i);

        ImGui::ColorEdit4("##color", &curve.color.x,
                           ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
                               ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoAlpha);
        ImGui::SameLine();
        ImGui::Checkbox("##enabled", &curve.enabled);
        ImGui::SameLine();

        ImGui::SetNextItemWidth(150);
        bool entered = ImGui::InputText("##expr", curve.exprBuf, sizeof(curve.exprBuf),
                                         ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::IsItemActive() || ImGui::IsItemActivated()) activeCurveIndex_ = i;
        if (entered) plotCurve(curve);

        ImGui::SameLine();
        if (ImGui::Checkbox("f'", &curve.showDerivative)) plotCurve(curve);
        ImGui::SameLine();
        if (ImGui::Checkbox("roots", &curve.showRoots)) plotCurve(curve);
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) pendingRemoval = i;

        if (!curve.error.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
            ImGui::TextWrapped("  %s", curve.error.c_str());
            ImGui::PopStyleColor();
        } else if (curve.showRoots && !curve.roots.empty()) {
            std::string rootsText = "  roots: ";
            for (size_t r = 0; r < curve.roots.size(); ++r) {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%.4g", curve.roots[r]);
                rootsText += buf;
                if (r + 1 < curve.roots.size()) rootsText += ", ";
            }
            ImGui::TextDisabled("%s", rootsText.c_str());
        }

        ImGui::PopID();
    }

    if (pendingRemoval >= 0) removeCurve(pendingRemoval);

    ImGui::BeginDisabled(static_cast<int>(curves_.size()) >= kMaxCurves);
    if (ImGui::Button("+ Add curve")) addCurve();
    ImGui::EndDisabled();
}

void CalculatorApp::renderRangeControls() {
    ImGui::SetNextItemWidth(90);
    ImGui::DragFloat("x min", &xMin_, 0.5f, -1000.0f, 1000.0f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::DragFloat("x max", &xMax_, 0.5f, -1000.0f, 1000.0f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::DragInt("samples", &samples_, 10, kMinSamples, kMaxSamples);

    if (ImGui::Button("Plot All", ImVec2(120, 32))) plotAll();
    ImGui::SameLine();
    if (ImGui::Button("Clear All", ImVec2(120, 32))) {
        for (auto& curve : curves_) {
            curve.data.clear();
            curve.derivative.clear();
            curve.roots.clear();
            curve.error.clear();
        }
        globalError_.clear();
    }

    if (!globalError_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
        ImGui::TextWrapped("Error: %s", globalError_.c_str());
        ImGui::PopStyleColor();
    }
}

void CalculatorApp::renderButtonPad() {
    static const char* buttons[] = {
        "pi", "log", "sqrt", "(",  ")",  "/",
        "sin", "cos", "tan", "7", "8", "9", "*",
        "asin", "acos", "atan", "4", "5", "6", "-",
        "exp", "abs", "^", "1", "2", "3", "+",
        "e", "x", ".", "0",
    };

    const int columns = 7;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    int count = 0;
    for (const char* label : buttons) {
        ImGui::PushID(label);
        if (ImGui::Button(label, ImVec2(56, 30))) insertText(label);
        ImGui::PopID();
        if (++count % columns != 0) ImGui::SameLine();
        else ImGui::NewLine();
    }
    ImGui::PopStyleVar();

    ImGui::Spacing();
    if (ImGui::Button("Backspace", ImVec2(128, 28)) && activeCurveIndex_ >= 0 &&
        activeCurveIndex_ < static_cast<int>(curves_.size())) {
        Curve& curve = curves_[activeCurveIndex_];
        std::string current(curve.exprBuf);
        if (!current.empty()) {
            current.pop_back();
            std::strncpy(curve.exprBuf, current.c_str(), sizeof(curve.exprBuf) - 1);
        }
    }
}

void CalculatorApp::renderIntegralPanel() {
    if (!ImGui::CollapsingHeader("Definite Integral")) return;

    ImGui::SetNextItemWidth(-1);
    std::string preview = curves_.empty() ? "(no curves)" : curveLabel(curves_[integral_.curveIndex], integral_.curveIndex);
    if (ImGui::BeginCombo("##integral_curve", preview.c_str())) {
        for (int i = 0; i < static_cast<int>(curves_.size()); ++i) {
            bool selected = (i == integral_.curveIndex);
            if (ImGui::Selectable(curveLabel(curves_[i], i).c_str(), selected)) {
                integral_.curveIndex = i;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SetNextItemWidth(90);
    ImGui::DragFloat("a", &integral_.a, 0.1f, -1000.0f, 1000.0f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::DragFloat("b", &integral_.b, 0.1f, -1000.0f, 1000.0f);

    ImGui::Checkbox("Shade region on plot", &integral_.showShading);

    ImGui::BeginDisabled(curves_.empty());
    if (ImGui::Button("Compute", ImVec2(120, 28))) computeIntegral();
    ImGui::EndDisabled();

    if (!integral_.error.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
        ImGui::TextWrapped("Error: %s", integral_.error.c_str());
        ImGui::PopStyleColor();
    } else if (integral_.hasResult) {
        ImGui::Text("Result (Simpson's rule): %.6f", integral_.result);
    }
}

void CalculatorApp::renderPlot() {
    if (ImPlot::BeginPlot("##graph", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("x", "f(x)");

        for (int i = 0; i < static_cast<int>(curves_.size()); ++i) {
            const Curve& curve = curves_[i];
            if (!curve.enabled || curve.data.size() == 0) continue;

            std::string label = curveLabel(curve, i);

            ImPlot::SetNextLineStyle(curve.color, 2.0f);
            ImPlot::PlotLine(label.c_str(), curve.data.xValues().data(), curve.data.yValues().data(),
                              static_cast<int>(curve.data.size()));

            if (curve.showDerivative && curve.derivative.size() > 0) {
                std::string derivLabel = label + " '";
                ImVec4 lighter = curve.color;
                lighter.w = 0.6f;
                ImPlot::SetNextLineStyle(lighter, 1.5f);
                ImPlot::PlotLine(derivLabel.c_str(), curve.derivative.xValues().data(),
                                  curve.derivative.yValues().data(),
                                  static_cast<int>(curve.derivative.size()));
            }

            if (curve.showRoots && !curve.roots.empty()) {
                std::vector<double> zeros(curve.roots.size(), 0.0);
                std::string rootsLabel = label + " roots";
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5.0f, curve.color, 1.5f, curve.color);
                ImPlot::PlotScatter(rootsLabel.c_str(), curve.roots.data(), zeros.data(),
                                     static_cast<int>(curve.roots.size()));
            }
        }

        if (integral_.showShading && integral_.hasResult && !integral_.shadeX.empty()) {
            ImVec4 fill(1.0f, 1.0f, 1.0f, 0.25f);
            if (integral_.curveIndex >= 0 && integral_.curveIndex < static_cast<int>(curves_.size())) {
                fill = curves_[integral_.curveIndex].color;
                fill.w = 0.30f;
            }
            ImPlot::SetNextFillStyle(fill);
            ImPlot::PlotShaded("integral region", integral_.shadeX.data(), integral_.shadeY.data(),
                                static_cast<int>(integral_.shadeX.size()), 0.0);
        }

        ImPlot::EndPlot();
    }
}

void CalculatorApp::Render() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("Graphing Calculator", nullptr,
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                      ImGuiWindowFlags_NoTitleBar);

    float leftWidth = viewport->WorkSize.x * 0.38f;
    ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, 0), true);
    renderCurveList();
    ImGui::Separator();
    renderRangeControls();
    ImGui::Separator();
    renderButtonPad();
    ImGui::Separator();
    renderIntegralPanel();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
    renderPlot();
    ImGui::EndChild();

    ImGui::End();
}
