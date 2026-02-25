/********************************************************************************************************

Authors:        (c) 2026 Maths Town

Licence:        The MIT License

*********************************************************************************************************
Float32 SIMD tests.
Uses FallbackFloat32 as the reference implementation.
*********************************************************************************************************/

#include "test_f32.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>

#include "../Include/simd-cpuid.h"
#include "../Include/simd-f32.h"

namespace {

enum class ArithmeticOp {
    add,
    sub,
    mul,
    div
};

bool float_matches_fallback(float actual, float expected) {
    if (std::isnan(actual) && std::isnan(expected)) {
        return true;
    }
    if (std::isinf(actual) || std::isinf(expected)) {
        return actual == expected;
    }

    const float abs_diff = std::fabs(actual - expected);
    const float scale = std::max(1.0f, std::max(std::fabs(actual), std::fabs(expected)));
    return abs_diff <= (2.0e-6f * scale);
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

float apply_fallback_op(float lhs, float rhs, ArithmeticOp op) {
    if (op == ArithmeticOp::add) {
        return (FallbackFloat32(lhs) + FallbackFloat32(rhs)).v;
    }
    if (op == ArithmeticOp::sub) {
        return (FallbackFloat32(lhs) - FallbackFloat32(rhs)).v;
    }
    if (op == ArithmeticOp::mul) {
        return (FallbackFloat32(lhs) * FallbackFloat32(rhs)).v;
    }
    return (FallbackFloat32(lhs) / FallbackFloat32(rhs)).v;
}

float sanitize_rhs_for_division(float rhs) {
    if (rhs == 0.0f || rhs == -0.0f) {
        return 1.0f;
    }
    return rhs;
}

template <typename SimdType>
bool run_float32_binary_test_for_type(
    const std::string& type_name,
    const std::string& op_name,
    ArithmeticOp op,
    CpuInformation cpu,
    TestHarness& harness) {
    const std::string test_name = type_name + " Float32 " + op_name;

    if (!SimdType::cpu_supported(cpu)) {
        return true;
    }

    if (!SimdType::compiler_supported()) {
        return true;
    }

    constexpr int lanes = SimdType::number_of_elements();
    std::mt19937 rng(20260224u);
    std::uniform_real_distribution<float> small_dist(-1000.0f, 1000.0f);
    std::uniform_real_distribution<float> tiny_dist(-1.0e-30f, 1.0e-30f);
    std::uniform_real_distribution<float> large_dist(-1.0e30f, 1.0e30f);

    // Randomized coverage with mixed scales so each lane sees distinct values.
    for (int iteration = 0; iteration < 1200; ++iteration) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};

        for (int lane = 0; lane < lanes; ++lane) {
            float lhs = 0.0f;
            float rhs = 0.0f;
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
            const float lhs = a.element(lane);
            const float rhs = b.element(lane);
            const float actual = result.element(lane);
            const float expected = apply_fallback_op(lhs, rhs, op);
            if (!float_matches_fallback(actual, expected)) {
                harness.add_result(
                    test_name,
                    false,
                    "Random case mismatch at lane " + std::to_string(lane));
                return false;
            }
        }
    }

    // Directed edge cases: zeros, signed values, tiny and large magnitudes.
    const float edge_values[] = {
        0.0f,
        -0.0f,
        1.0f,
        -1.0f,
        1.0e-30f,
        -1.0e-30f,
        1.0e30f,
        -1.0e30f,
        std::numeric_limits<float>::min(),
        -std::numeric_limits<float>::min(),
        std::numeric_limits<float>::max() * 0.25f,
        -std::numeric_limits<float>::max() * 0.25f
    };
    constexpr int edge_count = static_cast<int>(sizeof(edge_values) / sizeof(edge_values[0]));

    for (int base = 0; base < edge_count; ++base) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};
        for (int lane = 0; lane < lanes; ++lane) {
            const float lhs = edge_values[(base + lane) % edge_count];
            float rhs = edge_values[(base * 3 + lane + 1) % edge_count];
            if (op == ArithmeticOp::div) {
                rhs = sanitize_rhs_for_division(rhs);
            }
            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        alignas(SimdType) const SimdType result = apply_simd_op(a, b, op);
        for (int lane = 0; lane < lanes; ++lane) {
            const float lhs = a.element(lane);
            const float rhs = b.element(lane);
            const float actual = result.element(lane);
            const float expected = apply_fallback_op(lhs, rhs, op);
            if (!float_matches_fallback(actual, expected)) {
                harness.add_result(
                    test_name,
                    false,
                    "Edge case mismatch at lane " + std::to_string(lane));
                return false;
            }
        }
    }

    harness.add_result(test_name, true, "Random + edge cases matched fallback");
    return true;
}

template <typename SimdType>
bool run_float32_suite_for_type(const char* type_name, CpuInformation cpu, TestHarness& harness) {
    return run_float32_binary_test_for_type<SimdType>(type_name, "addition", ArithmeticOp::add, cpu, harness) &&
           run_float32_binary_test_for_type<SimdType>(type_name, "subtraction", ArithmeticOp::sub, cpu, harness) &&
           run_float32_binary_test_for_type<SimdType>(type_name, "multiplication", ArithmeticOp::mul, cpu, harness) &&
           run_float32_binary_test_for_type<SimdType>(type_name, "division", ArithmeticOp::div, cpu, harness);
}

} // namespace

void run_float32_arithmetic_tests(TestHarness& harness) {
    std::cout << "\n===== Float32 Arithmetic Tests =====\n";
    if (harness.should_halt()) {
        return;
    }
#if defined(_M_X64) || defined(__x86_64)
    CpuInformation cpu{};
#define MT_RUN_OR_HALT(expr) do { if (!(expr)) { return; } } while (false)
    MT_RUN_OR_HALT(run_float32_suite_for_type<Simd128Float32>("Simd128", cpu, harness));
#if MT_SIMD_ALLOW_LEVEL3_TYPES
    MT_RUN_OR_HALT(run_float32_suite_for_type<Simd256Float32>("Simd256", cpu, harness));
#endif
#if MT_SIMD_ALLOW_LEVEL4_TYPES
    MT_RUN_OR_HALT(run_float32_suite_for_type<Simd512Float32>("Simd512", cpu, harness));
#endif
#undef MT_RUN_OR_HALT
#endif
    std::cout << "==================================\n";
}





