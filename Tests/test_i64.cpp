/********************************************************************************************************

Authors:        (c) 2026 Maths Town

Licence:        The MIT License

*********************************************************************************************************
Int64 SIMD tests.
Uses FallbackInt64 as the reference implementation.
*********************************************************************************************************/

#include "test_i64.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <random>

#include "../Include/simd-cpuid.h"
#include "../Include/simd-int64.h"

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

int64_t apply_fallback_op(int64_t lhs, int64_t rhs, ArithmeticOp op) {
    if (op == ArithmeticOp::add) {
        return (FallbackInt64(lhs) + FallbackInt64(rhs)).v;
    }
    if (op == ArithmeticOp::sub) {
        return (FallbackInt64(lhs) - FallbackInt64(rhs)).v;
    }
    if (op == ArithmeticOp::mul) {
        return (FallbackInt64(lhs) * FallbackInt64(rhs)).v;
    }
    return (FallbackInt64(lhs) / FallbackInt64(rhs)).v;
}

int64_t sanitize_rhs_for_division(int64_t lhs, int64_t rhs) {
    if (rhs == 0) {
        return 1;
    }
    if (lhs == std::numeric_limits<int64_t>::min() && rhs == -1) {
        return 1;
    }
    return rhs;
}

template <typename SimdType>
bool run_int64_binary_test_for_type(
    const std::string& type_name,
    const std::string& op_name,
    ArithmeticOp op,
    CpuInformation cpu,
    TestHarness& harness) {
    const std::string test_name = type_name + " Int64 " + op_name;

    if (!SimdType::cpu_supported(cpu)) {
        return true;
    }

    if (!SimdType::compiler_supported()) {
        return true;
    }

    constexpr int lanes = SimdType::number_of_elements();
    std::mt19937_64 rng(20260224ull);
    std::uniform_int_distribution<int64_t> add_sub_dist(-1000000000000ll, 1000000000000ll);
    std::uniform_int_distribution<int64_t> mul_dist(-1000000ll, 1000000ll);
    std::uniform_int_distribution<int64_t> div_dist(-1000000000000ll, 1000000000000ll);

    for (int iteration = 0; iteration < 1200; ++iteration) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};

        for (int lane = 0; lane < lanes; ++lane) {
            int64_t lhs = 0;
            int64_t rhs = 0;

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
            const int64_t lhs = a.element(lane);
            const int64_t rhs = b.element(lane);
            const int64_t actual = result.element(lane);
            const int64_t expected = apply_fallback_op(lhs, rhs, op);
            if (actual != expected) {
                harness.add_result(test_name, false, "Random case mismatch at lane " + std::to_string(lane));
                return false;
            }
        }
    }

    const int64_t add_sub_edges[] = {0ll, 1ll, -1ll, 7ll, -7ll, 1234567ll, -2345678ll, 1000000000000ll, -1000000000000ll};
    const int64_t mul_edges[] = {0ll, 1ll, -1ll, 2ll, -2ll, 17ll, -17ll, 1000000ll, -1000000ll};
    const int64_t div_edges[] = {1ll, -1ll, 2ll, -2ll, 3ll, -3ll, 17ll, -17ll, 1000000000000ll, -1000000000000ll};
    const int64_t lhs_edges_div[] = {0ll, 1ll, -1ll, 7ll, -7ll, 1234567ll, -2345678ll, 1000000000000ll, -1000000000000ll};

    const int64_t* lhs_edges = (op == ArithmeticOp::div) ? lhs_edges_div : ((op == ArithmeticOp::mul) ? mul_edges : add_sub_edges);
    const int64_t* rhs_edges = (op == ArithmeticOp::div) ? div_edges : ((op == ArithmeticOp::mul) ? mul_edges : add_sub_edges);
    constexpr int edge_count = 9;

    for (int base = 0; base < edge_count; ++base) {
        alignas(SimdType) SimdType a{};
        alignas(SimdType) SimdType b{};
        for (int lane = 0; lane < lanes; ++lane) {
            const int64_t lhs = lhs_edges[(base + lane) % edge_count];
            int64_t rhs = rhs_edges[(base * 3 + lane + 1) % edge_count];
            if (op == ArithmeticOp::div) {
                rhs = sanitize_rhs_for_division(lhs, rhs);
            }
            a.set_element(lane, lhs);
            b.set_element(lane, rhs);
        }

        alignas(SimdType) SimdType result = apply_simd_op(a, b, op);
        for (int lane = 0; lane < lanes; ++lane) {
            const int64_t lhs = a.element(lane);
            const int64_t rhs = b.element(lane);
            const int64_t actual = result.element(lane);
            const int64_t expected = apply_fallback_op(lhs, rhs, op);
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
bool run_int64_suite_for_type(const char* type_name, CpuInformation cpu, TestHarness& harness) {
    return run_int64_binary_test_for_type<SimdType>(type_name, "addition", ArithmeticOp::add, cpu, harness) &&
           run_int64_binary_test_for_type<SimdType>(type_name, "subtraction", ArithmeticOp::sub, cpu, harness) &&
           run_int64_binary_test_for_type<SimdType>(type_name, "multiplication", ArithmeticOp::mul, cpu, harness) &&
           run_int64_binary_test_for_type<SimdType>(type_name, "division", ArithmeticOp::div, cpu, harness);
}

} // namespace

void run_int64_arithmetic_tests(TestHarness& harness) {
    std::cout << "\n===== Int64 Arithmetic Tests =====\n";
    if (harness.should_halt()) {
        return;
    }
#if defined(_M_X64) || defined(__x86_64)
    CpuInformation cpu{};
#define MT_RUN_OR_HALT(expr) do { if (!(expr)) { return; } } while (false)
    MT_RUN_OR_HALT(run_int64_suite_for_type<Simd128Int64>("Simd128", cpu, harness));
#if MT_SIMD_ALLOW_LEVEL3_TYPES
    MT_RUN_OR_HALT(run_int64_suite_for_type<Simd256Int64>("Simd256", cpu, harness));
#endif
#if MT_SIMD_ALLOW_LEVEL4_TYPES
    MT_RUN_OR_HALT(run_int64_suite_for_type<Simd512Int64>("Simd512", cpu, harness));
#endif
#undef MT_RUN_OR_HALT
#endif
    std::cout << "==================================\n";
}




