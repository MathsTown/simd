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
#include <string_view>

#include "../Include/simd-cpuid.h"
#include "../Include/simd-f32.h"

namespace {

enum class ArithmeticOp {
    add,
    sub,
    mul,
    div
};

enum class ArithmeticPath {
    vector_vector,
    vector_scalar_right,
    scalar_left_vector,
    compound_vector,
    compound_scalar
};

constexpr ArithmeticPath kArithmeticPaths[] = {
    ArithmeticPath::vector_vector,
    ArithmeticPath::vector_scalar_right,
    ArithmeticPath::scalar_left_vector,
    ArithmeticPath::compound_vector,
    ArithmeticPath::compound_scalar
};

std::string_view path_name(ArithmeticPath path) {
    if (path == ArithmeticPath::vector_vector) {
        return "vector op vector";
    }
    if (path == ArithmeticPath::vector_scalar_right) {
        return "vector op scalar";
    }
    if (path == ArithmeticPath::scalar_left_vector) {
        return "scalar op vector";
    }
    if (path == ArithmeticPath::compound_vector) {
        return "compound op vector";
    }
    return "compound op scalar";
}

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

float apply_fallback_binary(float lhs, float rhs, ArithmeticOp op) {
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

template <typename SimdType>
void apply_simd_op_with_path(
    const SimdType& a,
    const SimdType& b,
    float scalar,
    ArithmeticOp op,
    ArithmeticPath path,
    SimdType& out) {
    if (path == ArithmeticPath::vector_vector) {
        out = a;
        if (op == ArithmeticOp::add) { out += b; return; }
        if (op == ArithmeticOp::sub) { out -= b; return; }
        if (op == ArithmeticOp::mul) { out *= b; return; }
        out /= b;
        return;
    }
    if (path == ArithmeticPath::vector_scalar_right) {
        out = a;
        if (op == ArithmeticOp::add) { out += scalar; return; }
        if (op == ArithmeticOp::sub) { out -= scalar; return; }
        if (op == ArithmeticOp::mul) { out *= scalar; return; }
        out /= scalar;
        return;
    }
    if (path == ArithmeticPath::scalar_left_vector) {
        out = SimdType(scalar);
        if (op == ArithmeticOp::add) { out += b; return; }
        if (op == ArithmeticOp::sub) { out -= b; return; }
        if (op == ArithmeticOp::mul) { out *= b; return; }
        out /= b;
        return;
    }

    out = a;
    if (path == ArithmeticPath::compound_vector) {
        if (op == ArithmeticOp::add) { out += b; }
        else if (op == ArithmeticOp::sub) { out -= b; }
        else if (op == ArithmeticOp::mul) { out *= b; }
        else { out /= b; }
        return;
    }

    if (op == ArithmeticOp::add) { out += scalar; }
    else if (op == ArithmeticOp::sub) { out -= scalar; }
    else if (op == ArithmeticOp::mul) { out *= scalar; }
    else { out /= scalar; }
}

float apply_fallback_op_with_path(
    float lhs,
    float rhs,
    float scalar,
    ArithmeticOp op,
    ArithmeticPath path) {
    if (path == ArithmeticPath::vector_vector) {
        return apply_fallback_binary(lhs, rhs, op);
    }
    if (path == ArithmeticPath::vector_scalar_right) {
        return apply_fallback_binary(lhs, scalar, op);
    }
    if (path == ArithmeticPath::scalar_left_vector) {
        return apply_fallback_binary(scalar, rhs, op);
    }
    if (path == ArithmeticPath::compound_vector) {
        FallbackFloat32 out(lhs);
        if (op == ArithmeticOp::add) { out += FallbackFloat32(rhs); }
        else if (op == ArithmeticOp::sub) { out -= FallbackFloat32(rhs); }
        else if (op == ArithmeticOp::mul) { out *= FallbackFloat32(rhs); }
        else { out /= FallbackFloat32(rhs); }
        return out.v;
    }

    FallbackFloat32 out(lhs);
    if (op == ArithmeticOp::add) { out += scalar; }
    else if (op == ArithmeticOp::sub) { out -= scalar; }
    else if (op == ArithmeticOp::mul) { out *= scalar; }
    else { out /= scalar; }
    return out.v;
}

float finite_scalar_for_op(ArithmeticOp op) {
    if (op == ArithmeticOp::add) { return 3.75f; }
    if (op == ArithmeticOp::sub) { return -2.5f; }
    if (op == ArithmeticOp::mul) { return -1.25f; }
    return 2.0f;
}

template <typename SimdType>
bool run_float32_binary_test_for_type(
    const std::string& type_name,
    const std::string& op_name,
    ArithmeticOp op,
    CpuInformation cpu,
    TestHarness& harness) {
    const std::string test_name = type_name + " Float32 " + op_name;

    if (!SimdType::cpu_supported(cpu) || !SimdType::compiler_supported()) {
        return true;
    }

    constexpr int lanes = SimdType::number_of_elements();
    std::mt19937 rng(20260224u);
    std::uniform_real_distribution<float> small_dist(-1000.0f, 1000.0f);
    std::uniform_real_distribution<float> tiny_dist(-1.0e-30f, 1.0e-30f);
    std::uniform_real_distribution<float> large_dist(-1.0e30f, 1.0e30f);

    for (int iteration = 0; iteration < 1200; ++iteration) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};
        for (int lane = 0; lane < lanes; ++lane) {
            float lhs = 0.0f;
            float rhs = 1.0f;
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
            if (op == ArithmeticOp::div && rhs == 0.0f) {
                rhs = 1.0f;
            }
            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        for (const ArithmeticPath path : kArithmeticPaths) {
            const float scalar = finite_scalar_for_op(op);
            alignas(SimdType) SimdType result{};
            apply_simd_op_with_path(a, b, scalar, op, path, result);
            for (int lane = 0; lane < lanes; ++lane) {
                const float lhs = a.element(lane);
                const float rhs = b.element(lane);
                const float expected = apply_fallback_op_with_path(lhs, rhs, scalar, op, path);
                const float actual = result.element(lane);
                if (!float_matches_fallback(actual, expected)) {
                    harness.add_result(
                        test_name,
                        false,
                        "Random mismatch (" + std::string(path_name(path)) + "), lane " + std::to_string(lane));
                    return false;
                }
            }
        }
    }

    const float finite_edges[] = {
        0.0f,
        -0.0f,
        1.0f,
        -1.0f,
        std::numeric_limits<float>::denorm_min(),
        -std::numeric_limits<float>::denorm_min(),
        1.0e-30f,
        -1.0e-30f,
        1.0e30f,
        -1.0e30f,
        std::numeric_limits<float>::min(),
        -std::numeric_limits<float>::min(),
        std::numeric_limits<float>::max() * 0.5f,
        -std::numeric_limits<float>::max() * 0.5f
    };
    constexpr int finite_edge_count = static_cast<int>(sizeof(finite_edges) / sizeof(finite_edges[0]));

    for (int base = 0; base < finite_edge_count; ++base) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};
        for (int lane = 0; lane < lanes; ++lane) {
            const float lhs = finite_edges[(base + lane) % finite_edge_count];
            float rhs = finite_edges[(base * 3 + lane + 1) % finite_edge_count];
            if (op == ArithmeticOp::div && rhs == 0.0f) { rhs = 1.0f; }
            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        for (const ArithmeticPath path : kArithmeticPaths) {
            const float scalar = finite_scalar_for_op(op);
            alignas(SimdType) SimdType result{};
            apply_simd_op_with_path(a, b, scalar, op, path, result);
            for (int lane = 0; lane < lanes; ++lane) {
                const float lhs = a.element(lane);
                const float rhs = b.element(lane);
                const float expected = apply_fallback_op_with_path(lhs, rhs, scalar, op, path);
                const float actual = result.element(lane);
                if (!float_matches_fallback(actual, expected)) {
                    harness.add_result(
                        test_name,
                        false,
                        "Finite edge mismatch (" + std::string(path_name(path)) + "), lane " + std::to_string(lane));
                    return false;
                }
            }
        }
    }

    const float special_edges[] = {
        0.0f,
        -0.0f,
        1.0f,
        -1.0f,
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN(),
        -std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    };
    constexpr int special_edge_count = static_cast<int>(sizeof(special_edges) / sizeof(special_edges[0]));

    for (int base = 0; base < special_edge_count; ++base) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};
        for (int lane = 0; lane < lanes; ++lane) {
            const float lhs = special_edges[(base + lane) % special_edge_count];
            const float rhs = special_edges[(base * 5 + lane + 1) % special_edge_count];
            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        for (const ArithmeticPath path : kArithmeticPaths) {
            const float scalar = special_edges[(base * 7 + 3) % special_edge_count];
            alignas(SimdType) SimdType result{};
            apply_simd_op_with_path(a, b, scalar, op, path, result);
            for (int lane = 0; lane < lanes; ++lane) {
                const float lhs = a.element(lane);
                const float rhs = b.element(lane);
                const float expected = apply_fallback_op_with_path(lhs, rhs, scalar, op, path);
                const float actual = result.element(lane);
                if (!float_matches_fallback(actual, expected)) {
                    harness.add_result(
                        test_name,
                        false,
                        "Special edge mismatch (" + std::string(path_name(path)) + "), lane " + std::to_string(lane));
                    return false;
                }
            }
        }
    }

    harness.add_result(test_name, true, "Random + finite/special edges matched fallback");
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





