# Graphing Calculator

A lightweight, single-window desktop graphing calculator written in modern C++17. 

Type an expression in terms of `x`, hit **Plot**, and get an
interactive, pannable/zoomable graph — all in one native window, no external
processes.

Built with **[Dear ImGui](https://github.com/ocornut/imgui)** +
**[ImPlot](https://github.com/epezent/implot)** on **GLFW/OpenGL3**

## Features
- **Expression parser** (hand-written recursive shunting-yard, no external
  math library):
  - Operators `+ - * / ^`, correct precedence and associativity (`^` is
    right-associative).
  - Unary minus: `-x`, `-5`, `3 * -2` all work correctly.
  - Implicit multiplication: `2x`, `2(x+1)`, `x(x+1)`, `(x+1)(x-1)`.
  - Functions: `sin cos tan asin acos atan sinh cosh tanh sqrt exp abs log
    log10`.
  - Constants: `pi`, `e`.
  - Scientific notation: `1e-5`, `2.3E10`.
- **Safe evaluation** — malformed expressions, mismatched parentheses,
  division by zero, `sqrt` of a negative number, and stack underflow all
  raise a catchable `ExpressionError` with a human-readable message instead
  of crashing.
- **Multiple curves at once** — add up to 8 expressions, each with its own
  color, on/off toggle, and independent derivative/root settings, all
  plotted together with a legend.
- **Numerical methods** (in a standalone, independently testable
  `NumericalMethods` module):
  - **Derivative overlay** — central-difference approximation of `f'(x)`,
    plotted alongside `f(x)` when enabled per curve.
  - **Definite integral** — composite Simpson's rule over `[a, b]` for any
    curve, with the result shown numerically and the area optionally shaded
    on the plot.
- **Discontinuity-aware plotting** — non-finite results (e.g. `tan(x)` near
  an asymptote, `log(x)` for `x <= 0`) break the line instead of drawing a
  spurious vertical segment across the plot.
- **Adjustable view** — x-range and sample count are editable live, shared
  across all curves.
- **On-screen keypad** for numbers, operators, parentheses, and every
  supported function/constant, alongside a normal text field per curve —
  the keypad types into whichever curve's field you last clicked.

## Project layout
```
graphing-calculator/
├── CMakeLists.txt          # Fetches GLFW / Dear ImGui / ImPlot and builds the app
├── LICENSE
├── README.md
└── src/
    ├── main.cpp                     # GLFW + OpenGL3 + ImGui/ImPlot bootstrap and main loop
    ├── ExpressionEvaluator.h/.cpp   # Tokenizer, shunting-yard parser, postfix evaluator
    ├── NumericalMethods.h/.cpp      # Derivative, Simpson's-rule integral, bisection root finding
    ├── GraphData.h/.cpp             # Simple (x, y) sample container
    └── CalculatorApp.h/.cpp         # All UI: curve list, keypad, integral panel, plot

tests/
├── test_evaluator.cpp    # Standalone ExpressionEvaluator correctness checks
└── test_numerical.cpp    # Standalone NumericalMethods checks vs. closed-form answers
```

### 1. Install prerequisites

You need: a C++17 compiler, **CMake ≥ 3.16**, **Git** , and a system OpenGL installation.

**Ubuntu / Debian**
```bash
sudo apt update
sudo apt install build-essential cmake git \
    libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev
```

**Fedora**
```bash
sudo dnf install gcc-c++ cmake git mesa-libGL-devel \
    libX11-devel libXrandr-devel libXinerama-devel \
    libXcursor-devel libXi-devel
```

**macOS** (Xcode command line tools provide the compiler and OpenGL)
```bash
xcode-select --install
brew install cmake git
```

**Windows**
- Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with the
  "Desktop development with C++" workload (this gives you the compiler and
  OpenGL).
- Install [CMake](https://cmake.org/download/) and [Git](https://git-scm.com/download/win),
  making sure to check "Add to PATH" during setup.

### 2. Configure and build

This single pair of commands downloads GLFW, Dear ImGui, and ImPlot
automatically and compiles everything, dependencies included:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```


On Windows with Visual Studio, you can instead just open the cloned folder
in **Visual Studio 2022 → "Open Folder"**; it detects `CMakeLists.txt`
automatically and configures/builds via the same `FetchContent` process.

### 4. Run it

```bash
./build/GraphingCalculator                # Linux / macOS
build\Release\GraphingCalculator.exe      # Windows (multi-config generators)
```

## Usage

1. Type an expression into a curve's field, or build one with the keypad —
   e.g. `sin(x) + 0.5x`, `sqrt(x^2 - 1)`, `1/(x-2)`. The keypad always types
   into whichever field you last clicked.
2. Click **+ Add curve** to plot more than one function at once (up to 8) —
   each gets its own color, shown as a swatch next to its field.
3. Per curve, toggle:
   - **f'** to overlay its numerical derivative (central difference), drawn
     in the same color at reduced opacity.
   - **roots** to mark its x-intercepts (found via bisection) and list their
     values underneath the field.
4. Adjust `x min`, `x max`, and `samples` — these apply to every curve —
   then click **Plot All** (or press Enter in any single curve's field to
   replot just that one).
5. Open **Definite Integral**, pick a curve, set bounds `a` and `b`, and
   click **Compute** to get the value via Simpson's rule; check **Shade
   region on plot** to visualize the area.
6. On the graph: scroll to zoom, drag to pan, double-click to reset the
   view, hover a point to read its coordinates — all standard ImPlot
   interactions.

## Testing

`ExpressionEvaluator` and `NumericalMethods` are deliberately free of any
GUI dependency, so they're tested with small standalone programs — no
GTest/Catch2 needed, and no need to build the full GLFW/ImGui/ImPlot stack
just to check the math:

```bash
cd tests
g++ -std=c++17 -Wall -Wextra ../src/ExpressionEvaluator.cpp test_evaluator.cpp -o test_evaluator -I.. && ./test_evaluator
g++ -std=c++17 -Wall -Wextra ../src/ExpressionEvaluator.cpp ../src/NumericalMethods.cpp test_numerical.cpp -o test_numerical -I.. && ./test_numerical
```



