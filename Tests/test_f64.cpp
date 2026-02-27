/********************************************************************************************************

Authors:        (c) 2026 Maths Town

Licence:        The MIT License

*********************************************************************************************************
Float64 SIMD tests.
Uses FallbackFloat64 as the reference implementation.
*********************************************************************************************************/

#include "test_f64.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

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

double apply_fallback_binary(double lhs, double rhs, ArithmeticOp op) {
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

template <typename SimdType>
void apply_simd_op_with_path(
    const SimdType& a,
    const SimdType& b,
    double scalar,
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

double apply_fallback_op_with_path(
    double lhs,
    double rhs,
    double scalar,
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
        FallbackFloat64 out(lhs);
        if (op == ArithmeticOp::add) { out += FallbackFloat64(rhs); }
        else if (op == ArithmeticOp::sub) { out -= FallbackFloat64(rhs); }
        else if (op == ArithmeticOp::mul) { out *= FallbackFloat64(rhs); }
        else { out /= FallbackFloat64(rhs); }
        return out.v;
    }

    FallbackFloat64 out(lhs);
    if (op == ArithmeticOp::add) { out += scalar; }
    else if (op == ArithmeticOp::sub) { out -= scalar; }
    else if (op == ArithmeticOp::mul) { out *= scalar; }
    else { out /= scalar; }
    return out.v;
}

double finite_scalar_for_op(ArithmeticOp op) {
    if (op == ArithmeticOp::add) { return 7.5; }
    if (op == ArithmeticOp::sub) { return -2.25; }
    if (op == ArithmeticOp::mul) { return -1.5; }
    return 2.0;
}

template <typename SimdType>
bool run_float64_binary_test_for_type(
    const std::string& type_name,
    const std::string& op_name,
    ArithmeticOp op,
    CpuInformation cpu,
    TestHarness& harness) {
    const std::string test_name = type_name + " Float64 " + op_name;

    if (!SimdType::cpu_supported(cpu) || !SimdType::compiler_supported()) {
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
            double rhs = 1.0;
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

            if (op == ArithmeticOp::div && rhs == 0.0) { rhs = 1.0; }

            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        for (const ArithmeticPath path : kArithmeticPaths) {
            const double scalar = finite_scalar_for_op(op);
            alignas(SimdType) SimdType result{};
            apply_simd_op_with_path(a, b, scalar, op, path, result);
            for (int lane = 0; lane < lanes; ++lane) {
                const double lhs = a.element(lane);
                const double rhs = b.element(lane);
                const double expected = apply_fallback_op_with_path(lhs, rhs, scalar, op, path);
                const double actual = result.element(lane);
                if (!double_matches_fallback(actual, expected)) {
                    harness.add_result(
                        test_name,
                        false,
                        "Random mismatch (" + std::string(path_name(path)) + "), lane " + std::to_string(lane));
                    return false;
                }
            }
        }
    }

    const double finite_edges[] = {
        0.0,
        -0.0,
        1.0,
        -1.0,
        std::numeric_limits<double>::denorm_min(),
        -std::numeric_limits<double>::denorm_min(),
        1.0e-200,
        -1.0e-200,
        1.0e200,
        -1.0e200,
        std::numeric_limits<double>::min(),
        -std::numeric_limits<double>::min(),
        std::numeric_limits<double>::max() * 0.5,
        -std::numeric_limits<double>::max() * 0.5
    };
    constexpr int finite_edge_count = static_cast<int>(sizeof(finite_edges) / sizeof(finite_edges[0]));

    for (int base = 0; base < finite_edge_count; ++base) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};
        for (int lane = 0; lane < lanes; ++lane) {
            const double lhs = finite_edges[(base + lane) % finite_edge_count];
            double rhs = finite_edges[(base * 3 + lane + 1) % finite_edge_count];
            if (op == ArithmeticOp::div && rhs == 0.0) { rhs = 1.0; }
            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        for (const ArithmeticPath path : kArithmeticPaths) {
            const double scalar = finite_scalar_for_op(op);
            alignas(SimdType) SimdType result{};
            apply_simd_op_with_path(a, b, scalar, op, path, result);
            for (int lane = 0; lane < lanes; ++lane) {
                const double lhs = a.element(lane);
                const double rhs = b.element(lane);
                const double expected = apply_fallback_op_with_path(lhs, rhs, scalar, op, path);
                const double actual = result.element(lane);
                if (!double_matches_fallback(actual, expected)) {
                    harness.add_result(
                        test_name,
                        false,
                        "Finite edge mismatch (" + std::string(path_name(path)) + "), lane " + std::to_string(lane));
                    return false;
                }
            }
        }
    }

    const double special_edges[] = {
        0.0,
        -0.0,
        1.0,
        -1.0,
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN(),
        -std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::max(),
        -std::numeric_limits<double>::max()
    };
    constexpr int special_edge_count = static_cast<int>(sizeof(special_edges) / sizeof(special_edges[0]));

    for (int base = 0; base < special_edge_count; ++base) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};
        for (int lane = 0; lane < lanes; ++lane) {
            const double lhs = special_edges[(base + lane) % special_edge_count];
            const double rhs = special_edges[(base * 5 + lane + 1) % special_edge_count];
            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        for (const ArithmeticPath path : kArithmeticPaths) {
            const double scalar = special_edges[(base * 7 + 3) % special_edge_count];
            alignas(SimdType) SimdType result{};
            apply_simd_op_with_path(a, b, scalar, op, path, result);
            for (int lane = 0; lane < lanes; ++lane) {
                const double lhs = a.element(lane);
                const double rhs = b.element(lane);
                const double expected = apply_fallback_op_with_path(lhs, rhs, scalar, op, path);
                const double actual = result.element(lane);
                if (!double_matches_fallback(actual, expected)) {
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





