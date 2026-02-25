/********************************************************************************************************

Authors:        (c) 2026 Maths Town

Licence:        The MIT License

*********************************************************************************************************
Float64 SIMD tests.
Uses FallbackFloat64 as the reference implementation.
*********************************************************************************************************/

#include "test_f64.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>

#include "../Include/simd-cpuid.h"
#include "../Include/simd-concepts.h"
#include "../Include/simd-uint32.h"
#include "../Include/simd-uint64.h"
#include "../Include/simd-f64.h"

namespace {

enum class ArithmeticOp {
    add,
    sub,
    mul,
    div
};

bool double_matches_fallback(double actual, double expected) {
    if (std::isnan(actual) && std::isnan(expected)) {
        return true;
    }
    if (std::isinf(actual) || std::isinf(expected)) {
        return actual == expected;
    }

    const double abs_diff = std::fabs(actual - expected);
    const double scale = std::max(1.0, std::max(std::fabs(actual), std::fabs(expected)));
    return abs_diff <= (1.0e-12 * scale);
}

template <typename SimdType>
SimdType apply_simd_op(const SimdType& a, const SimdType& b, ArithmeticOp op) {
    if (op == ArithmeticOp::add) {
        return a + b;
    }
    if (op == ArithmeticOp::sub) {
        return a - b;
    }
    if (op == ArithmeticOp::mul) {
        return a * b;
    }
    return a / b;
}

double apply_fallback_op(double lhs, double rhs, ArithmeticOp op) {
    if (op == ArithmeticOp::add) {
        return (FallbackFloat64(lhs) + FallbackFloat64(rhs)).v;
    }
    if (op == ArithmeticOp::sub) {
        return (FallbackFloat64(lhs) - FallbackFloat64(rhs)).v;
    }
    if (op == ArithmeticOp::mul) {
        return (FallbackFloat64(lhs) * FallbackFloat64(rhs)).v;
    }
    return (FallbackFloat64(lhs) / FallbackFloat64(rhs)).v;
}

double sanitize_rhs_for_division(double rhs) {
    if (rhs == 0.0 || rhs == -0.0) {
        return 1.0;
    }
    return rhs;
}

template <typename SimdType>
bool run_float64_binary_test_for_type(
    const std::string& type_name,
    const std::string& op_name,
    ArithmeticOp op,
    CpuInformation cpu,
    TestHarness& harness) {
    const std::string test_name = type_name + " Float64 " + op_name;

    if (!SimdType::cpu_supported(cpu)) {
        return true;
    }

    if (!SimdType::compiler_supported()) {
        return true;
    }

    constexpr int lanes = SimdType::number_of_elements();
    std::mt19937_64 rng(20260224ull);
    std::uniform_real_distribution<double> small_dist(-1.0e6, 1.0e6);
    std::uniform_real_distribution<double> tiny_dist(-1.0e-200, 1.0e-200);
    std::uniform_real_distribution<double> large_dist(-1.0e200, 1.0e200);

    for (int iteration = 0; iteration < 1000; ++iteration) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};

        for (int lane = 0; lane < lanes; ++lane) {
            double lhs = 0.0;
            double rhs = 0.0;
            const int selector = (iteration + lane) % 3;

            if (selector == 0) {
                lhs = small_dist(rng);
                rhs = small_dist(rng);
            } else if (selector == 1) {
                lhs = tiny_dist(rng);
                rhs = large_dist(rng);
            } else {
                lhs = large_dist(rng);
                rhs = tiny_dist(rng);
            }

            if (op == ArithmeticOp::div) {
                rhs = sanitize_rhs_for_division(rhs);
            }

            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        alignas(SimdType) const SimdType result = apply_simd_op(a, b, op);
        for (int lane = 0; lane < lanes; ++lane) {
            const double lhs = a.element(lane);
            const double rhs = b.element(lane);
            const double actual = result.element(lane);
            const double expected = apply_fallback_op(lhs, rhs, op);
            if (!double_matches_fallback(actual, expected)) {
                harness.add_result(test_name, false, "Random case mismatch at lane " + std::to_string(lane));
                return false;
            }
        }
    }

    const double edge_values[] = {
        0.0,
        -0.0,
        1.0,
        -1.0,
        1.0e-200,
        -1.0e-200,
        1.0e200,
        -1.0e200,
        std::numeric_limits<double>::min(),
        -std::numeric_limits<double>::min(),
        std::numeric_limits<double>::max() * 0.25,
        -std::numeric_limits<double>::max() * 0.25
    };
    constexpr int edge_count = static_cast<int>(sizeof(edge_values) / sizeof(edge_values[0]));

    for (int base = 0; base < edge_count; ++base) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};
        for (int lane = 0; lane < lanes; ++lane) {
            const double lhs = edge_values[(base + lane) % edge_count];
            double rhs = edge_values[(base * 3 + lane + 1) % edge_count];
            if (op == ArithmeticOp::div) {
                rhs = sanitize_rhs_for_division(rhs);
            }
            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        alignas(SimdType) const SimdType result = apply_simd_op(a, b, op);
        for (int lane = 0; lane < lanes; ++lane) {
            const double lhs = a.element(lane);
            const double rhs = b.element(lane);
            const double actual = result.element(lane);
            const double expected = apply_fallback_op(lhs, rhs, op);
            if (!double_matches_fallback(actual, expected)) {
                harness.add_result(test_name, false, "Edge case mismatch at lane " + std::to_string(lane));
                return false;
            }
        }
    }

    harness.add_result(test_name, true, "Random + edge cases matched fallback");
    return true;
}

template <typename SimdType>
bool run_float64_suite_for_type(const char* type_name, CpuInformation cpu, TestHarness& harness) {
    return run_float64_binary_test_for_type<SimdType>(type_name, "addition", ArithmeticOp::add, cpu, harness) &&
           run_float64_binary_test_for_type<SimdType>(type_name, "subtraction", ArithmeticOp::sub, cpu, harness) &&
           run_float64_binary_test_for_type<SimdType>(type_name, "multiplication", ArithmeticOp::mul, cpu, harness) &&
           run_float64_binary_test_for_type<SimdType>(type_name, "division", ArithmeticOp::div, cpu, harness);
}

} // namespace

void run_float64_arithmetic_tests(TestHarness& harness) {
    std::cout << "\n===== Float64 Arithmetic Tests =====\n";
    if (harness.should_halt()) {
        return;
    }
#if defined(_M_X64) || defined(__x86_64)
    CpuInformation cpu{};
#define MT_RUN_OR_HALT(expr) do { if (!(expr)) { return; } } while (false)
    MT_RUN_OR_HALT(run_float64_suite_for_type<Simd128Float64>("Simd128", cpu, harness));
#if MT_SIMD_ALLOW_LEVEL3_TYPES
    MT_RUN_OR_HALT(run_float64_suite_for_type<Simd256Float64>("Simd256", cpu, harness));
#endif
#if MT_SIMD_ALLOW_LEVEL4_TYPES
    MT_RUN_OR_HALT(run_float64_suite_for_type<Simd512Float64>("Simd512", cpu, harness));
#endif
#undef MT_RUN_OR_HALT
#endif
    std::cout << "==================================\n";
}





