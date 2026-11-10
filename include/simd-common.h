#pragma once
/********************************************************************************************************

Authors:        (c) 2026 Maths Town

Licence:        The MIT License

*********************************************************************************************************
Shared compare, blend, and branchless conditional helpers, and the memory helpers written once over the types'
loads, stores, gathers and scatters.
*********************************************************************************************************/

#include <cmath>
#include <concepts>
#include <cstdint>
#include <cstring>

#include "simd-concepts.h"

namespace mt {

template <typename T>
concept GenericComparable = requires (const T a, const T b) {
	{ a == b } -> std::convertible_to<bool>;
	{ a < b } -> std::convertible_to<bool>;
	{ a <= b } -> std::convertible_to<bool>;
	{ a > b } -> std::convertible_to<bool>;
	{ a >= b } -> std::convertible_to<bool>;
};

template <typename T>
concept GenericBlendable = requires (const T if_false, const T if_true, bool mask) {
	{ mask ? if_true : if_false } -> std::convertible_to<T>;
};

template <GenericComparable T>
inline static bool compare_equal(const T a, const T b) noexcept(noexcept(a == b)) {
	return a == b;
}

template <GenericComparable T>
inline static bool compare_less(const T a, const T b) noexcept(noexcept(a < b)) {
	return a < b;
}

template <GenericComparable T>
inline static bool compare_less_equal(const T a, const T b) noexcept(noexcept(a <= b)) {
	return a <= b;
}

template <GenericComparable T>
inline static bool compare_greater(const T a, const T b) noexcept(noexcept(a > b)) {
	return a > b;
}

template <GenericComparable T>
inline static bool compare_greater_equal(const T a, const T b) noexcept(noexcept(a >= b)) {
	return a >= b;
}

template <GenericBlendable T>
[[nodiscard("Value Calculated and not used (blend)")]]
inline static T blend(const T if_false, const T if_true, bool mask) noexcept(noexcept(mask ? if_true : if_false)) {
	return mask ? if_true : if_false;
}

template <typename T>
requires requires (const T value_a, const T value_b, const T if_true, const T if_false) {
	compare_equal(value_a, value_b);
	{ blend(if_false, if_true, compare_equal(value_a, value_b)) } -> std::same_as<T>;
}
[[nodiscard("Value Calculated and not used (if_equal)")]]
inline static T if_equal(const T value_a, const T value_b, const T if_true, const T if_false) noexcept(noexcept(blend(if_false, if_true, compare_equal(value_a, value_b)))) {
	return blend(if_false, if_true, compare_equal(value_a, value_b));
}

template <typename T>
requires requires (const T value_a, const T value_b, const T if_true, const T if_false) {
	compare_less(value_a, value_b);
	{ blend(if_false, if_true, compare_less(value_a, value_b)) } -> std::same_as<T>;
}
[[nodiscard("Value Calculated and not used (if_less)")]]
inline static T if_less(const T value_a, const T value_b, const T if_true, const T if_false) noexcept(noexcept(blend(if_false, if_true, compare_less(value_a, value_b)))) {
	return blend(if_false, if_true, compare_less(value_a, value_b));
}

template <typename T>
requires requires (const T value_a, const T value_b, const T if_true, const T if_false) {
	compare_less_equal(value_a, value_b);
	{ blend(if_false, if_true, compare_less_equal(value_a, value_b)) } -> std::same_as<T>;
}
[[nodiscard("Value Calculated and not used (if_less_equal)")]]
inline static T if_less_equal(const T value_a, const T value_b, const T if_true, const T if_false) noexcept(noexcept(blend(if_false, if_true, compare_less_equal(value_a, value_b)))) {
	return blend(if_false, if_true, compare_less_equal(value_a, value_b));
}

template <typename T>
requires requires (const T value_a, const T value_b, const T if_true, const T if_false) {
	compare_greater(value_a, value_b);
	{ blend(if_false, if_true, compare_greater(value_a, value_b)) } -> std::same_as<T>;
}
[[nodiscard("Value Calculated and not used (if_greater)")]]
inline static T if_greater(const T value_a, const T value_b, const T if_true, const T if_false) noexcept(noexcept(blend(if_false, if_true, compare_greater(value_a, value_b)))) {
	return blend(if_false, if_true, compare_greater(value_a, value_b));
}

template <typename T>
requires requires (const T value_a, const T value_b, const T if_true, const T if_false) {
	compare_greater_equal(value_a, value_b);
	{ blend(if_false, if_true, compare_greater_equal(value_a, value_b)) } -> std::same_as<T>;
}
[[nodiscard("Value Calculated and not used (if_greater_equal)")]]
inline static T if_greater_equal(const T value_a, const T value_b, const T if_true, const T if_false) noexcept(noexcept(blend(if_false, if_true, compare_greater_equal(value_a, value_b)))) {
	return blend(if_false, if_true, compare_greater_equal(value_a, value_b));
}

template <std::floating_point T>
inline static bool isnan(const T value) noexcept {
	return std::isnan(value);
}

template <typename T>
requires requires (const T value_a, const T if_true, const T if_false) {
	isnan(value_a);
	{ blend(if_false, if_true, isnan(value_a)) } -> std::same_as<T>;
}
[[nodiscard("Value Calculated and not used (if_nan)")]]
inline static T if_nan(const T value_a, const T if_true, const T if_false) noexcept(noexcept(blend(if_false, if_true, isnan(value_a)))) {
	return blend(if_false, if_true, isnan(value_a));
}

namespace simd_detail {

	// The element at the x86 gather's address: base byte address + signed_index * Scale.
	template <int Scale, typename E, typename Index>
	inline E gather_element(const E* base, Index index) noexcept {
		static_assert(Scale == 1 || Scale == 2 || Scale == 4 || Scale == 8, "Gather scale must be 1, 2, 4, or 8.");
		const auto address = reinterpret_cast<std::uintptr_t>(base) + static_cast<std::uintptr_t>(static_cast<int64_t>(index) * Scale);
		E value;
		std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(value));
		return value;
	}

	// The first count lanes from memory through a zeroed temporary, for the types without a masked load.
	template <typename T>
	inline T load_first_lanes(const typename T::F* source, int count) noexcept {
		alignas(T) typename T::F lanes[T::number_of_elements()]{};
		std::memcpy(lanes, source, static_cast<std::size_t>(count) * sizeof(typename T::F));
		return T::make_load(lanes);
	}

	// The first count lanes to memory through a temporary, for the types without a masked store.
	template <typename T>
	inline void store_first_lanes(const T& value, typename T::F* destination, int count) noexcept {
		alignas(T) typename T::F lanes[T::number_of_elements()];
		value.store(lanes);
		std::memcpy(destination, lanes, static_cast<std::size_t>(count) * sizeof(typename T::F));
	}

	// A scatter without the instruction: the first count lanes stored one at a time in lane order, so two lanes with
	// one address leave the higher lane's value, as the x86 scatter does.
	template <int Scale, typename T>
	inline void scatter_lanes(const T& value, typename T::F* base, const typename T::I& indices, int count) noexcept {
		static_assert(Scale == 1 || Scale == 2 || Scale == 4 || Scale == 8, "Scatter scale must be 1, 2, 4, or 8.");
		alignas(T) typename T::F lanes[T::number_of_elements()];
		alignas(typename T::I) typename T::I::F offsets[T::number_of_elements()];
		value.store(lanes);
		indices.store(offsets);
		for (int i = 0; i < count; ++i) {
			const auto address = reinterpret_cast<std::uintptr_t>(base) + static_cast<std::uintptr_t>(static_cast<int64_t>(offsets[i]) * Scale);
			std::memcpy(reinterpret_cast<void*>(address), &lanes[i], sizeof(lanes[i]));
		}
	}

} // namespace simd_detail

/**************************************************************************************************
* Records of K elements laid end to end in memory, as K vectors: streams[k] lane i is source[i * K + k].
* Written over the type's gathers and scatters, so any K is served.
*************************************************************************************************/
template <int K, SimdGather T>
inline void load_interleaved(const typename T::F* source, T (&streams)[K]) noexcept {
	const typename T::I indices = T::I::make_sequential(0) * K;
	for (int k = 0; k < K; ++k) {
		streams[k] = T::template make_gather<static_cast<int>(sizeof(typename T::F))>(source + k, indices);
	}
}

template <int K, SimdGather T>
inline void store_interleaved(typename T::F* destination, const T (&streams)[K]) noexcept {
	const typename T::I indices = T::I::make_sequential(0) * K;
	for (int k = 0; k < K; ++k) {
		streams[k].template scatter<static_cast<int>(sizeof(typename T::F))>(destination + k, indices);
	}
}

} // namespace mt
