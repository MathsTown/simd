/********************************************************************************************************

Authors:        (c) 2026 Maths Town

Licence:        The MIT License

*********************************************************************************************************
Int32 SIMD tests.
Uses FallbackInt32 as the reference implementation.
*********************************************************************************************************/

#include "test_i32.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <random>

#include "../Include/simd-cpuid.h"
#include "../Include/simd-int32.h"

namespace {

enum class ArithmeticOp {
    add,
    sub,
    mul,
    div
};

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

int32_t apply_fallback_op(int32_t lhs, int32_t rhs, ArithmeticOp op) {
    if (op == ArithmeticOp::add) {
        return (FallbackInt32(lhs) + FallbackInt32(rhs)).v;
    }
    if (op == ArithmeticOp::sub) {
        return (FallbackInt32(lhs) - FallbackInt32(rhs)).v;
    }
    if (op == ArithmeticOp::mul) {
        return (FallbackInt32(lhs) * FallbackInt32(rhs)).v;
    }
    return (FallbackInt32(lhs) / FallbackInt32(rhs)).v;
}

int32_t sanitize_rhs_for_division(int32_t lhs, int32_t rhs) {
    if (rhs == 0) {
        return 1;
    }
    if (lhs == std::numeric_limits<int32_t>::min() && rhs == -1) {
        return 1;
    }
    return rhs;
}

template <typename SimdType>
bool run_int32_binary_test_for_type(
    const std::string& type_name,
    const std::string& op_name,
    ArithmeticOp op,
    CpuInformation cpu,
    TestHarness& harness) {
    const std::string test_name = type_name + " Int32 " + op_name;

    if (!SimdType::cpu_supported(cpu)) {
        return true;
    }

    if (!SimdType::compiler_supported()) {
        return true;
    }

    constexpr int lanes = SimdType::number_of_elements();
    std::mt19937 rng(20260224u);
    std::uniform_int_distribution<int32_t> add_sub_dist(-1000000, 1000000);
    std::uniform_int_distribution<int32_t> mul_dist(-1000, 1000);
    std::uniform_int_distribution<int32_t> div_dist(-1000000, 1000000);

    for (int iteration = 0; iteration < 1200; ++iteration) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};

        for (int lane = 0; lane < lanes; ++lane) {
            int32_t lhs = 0;
            int32_t rhs = 0;

            if (op == ArithmeticOp::mul) {
                lhs = mul_dist(rng);
                rhs = mul_dist(rng);
            } else if (op == ArithmeticOp::div) {
                lhs = div_dist(rng);
                rhs = div_dist(rng);
                rhs = sanitize_rhs_for_division(lhs, rhs);
            } else {
                lhs = add_sub_dist(rng);
                rhs = add_sub_dist(rng);
            }

            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        alignas(SimdType) SimdType result = apply_simd_op(a, b, op);
        for (int lane = 0; lane < lanes; ++lane) {
            const int32_t lhs = a.element(lane);
            const int32_t rhs = b.element(lane);
            const int32_t actual = result.element(lane);
            const int32_t expected = apply_fallback_op(lhs, rhs, op);
            if (actual != expected) {
                harness.add_result(test_name, false, "Random case mismatch at lane " + std::to_string(lane));
                return false;
            }
        }
    }

    const int32_t add_sub_edges[] = {0, 1, -1, 7, -7, 12345, -23456, 999999, -999999};
    const int32_t mul_edges[] = {0, 1, -1, 2, -2, 17, -17, 1000, -1000};
    const int32_t div_edges[] = {1, -1, 2, -2, 3, -3, 17, -17, 999999, -999999};
    const int32_t lhs_edges_div[] = {0, 1, -1, 7, -7, 12345, -23456, 999999, -999999};

    const int32_t* lhs_edges = (op == ArithmeticOp::div) ? lhs_edges_div : ((op == ArithmeticOp::mul) ? mul_edges : add_sub_edges);
    const int32_t* rhs_edges = (op == ArithmeticOp::div) ? div_edges : ((op == ArithmeticOp::mul) ? mul_edges : add_sub_edges);
    constexpr int edge_count = 9;

    for (int base = 0; base < edge_count; ++base) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};
        for (int lane = 0; lane < lanes; ++lane) {
            const int32_t lhs = lhs_edges[(base + lane) % edge_count];
            int32_t rhs = rhs_edges[(base * 3 + lane + 1) % edge_count];
            if (op == ArithmeticOp::div) {
                rhs = sanitize_rhs_for_division(lhs, rhs);
            }
            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        alignas(SimdType) SimdType result = apply_simd_op(a, b, op);
        for (int lane = 0; lane < lanes; ++lane) {
            const int32_t lhs = a.element(lane);
            const int32_t rhs = b.element(lane);
            const int32_t actual = result.element(lane);
            const int32_t expected = apply_fallback_op(lhs, rhs, op);
            if (actual != expected) {
                harness.add_result(test_name, false, "Edge case mismatch at lane " + std::to_string(lane));
                return false;
            }
        }
    }

    harness.add_result(test_name, true, "Random + edge cases matched fallback");
    return true;
}

template <typename SimdType>
bool run_int32_suite_for_type(const char* type_name, CpuInformation cpu, TestHarness& harness) {
    return run_int32_binary_test_for_type<SimdType>(type_name, "addition", ArithmeticOp::add, cpu, harness) &&
           run_int32_binary_test_for_type<SimdType>(type_name, "subtraction", ArithmeticOp::sub, cpu, harness) &&
           run_int32_binary_test_for_type<SimdType>(type_name, "multiplication", ArithmeticOp::mul, cpu, harness) &&
           run_int32_binary_test_for_type<SimdType>(type_name, "division", ArithmeticOp::div, cpu, harness);
}

} // namespace

void run_int32_arithmetic_tests(TestHarness& harness) {
    std::cout << "\n===== Int32 Arithmetic Tests =====\n";
    if (harness.should_halt()) {
        return;
    }
#if defined(_M_X64) || defined(__x86_64)
    CpuInformation cpu{};
#define MT_RUN_OR_HALT(expr) do { if (!(expr)) { return; } } while (false)
    MT_RUN_OR_HALT(run_int32_suite_for_type<Simd128Int32>("Simd128", cpu, harness));
#if MT_SIMD_ALLOW_LEVEL3_TYPES
    MT_RUN_OR_HALT(run_int32_suite_for_type<Simd256Int32>("Simd256", cpu, harness));
#endif
#if MT_SIMD_ALLOW_LEVEL4_TYPES
    MT_RUN_OR_HALT(run_int32_suite_for_type<Simd512Int32>("Simd512", cpu, harness));
#endif
#undef MT_RUN_OR_HALT
#endif
    std::cout << "==================================\n";
}




