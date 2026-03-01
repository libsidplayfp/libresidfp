#ifndef FPM_FIXED_HPP
#define FPM_FIXED_HPP

#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>

#ifndef FPM_NODISCARD
#   if __cplusplus >= 201703L /* C++17 */
#       define FPM_NODISCARD [[nodiscard]]
#   else
#       define FPM_NODISCARD
#   endif
#endif

namespace fpm
{

//! Fixed-point number type
//! \tparam BaseType         the base integer type used to store the fixed-point number. This can be a signed or unsigned type.
//! \tparam IntermediateType the integer type used to store intermediate results during calculations.
//! \tparam FractionBits     the number of bits of the BaseType used to store the fraction
//! \tparam EnableRounding   enable rounding of LSB for multiplication, division, and type conversion
template <typename BaseType, typename IntermediateType, unsigned int FractionBits, bool EnableRounding = true>
struct fixed
{
    static_assert(std::is_integral<BaseType>::value, "BaseType must be an integral type");
    static_assert(FractionBits > 0, "FractionBits must be greater than zero");
    static_assert(FractionBits <= sizeof(BaseType) * 8 - (std::numeric_limits<BaseType>::is_signed ? 1 : 0), "BaseType must at least be able to contain entire fraction, with space for at least one integral bit");
    static_assert(sizeof(IntermediateType) > sizeof(BaseType), "IntermediateType must be larger than BaseType");
    static_assert(std::numeric_limits<IntermediateType>::is_signed == std::numeric_limits<BaseType>::is_signed, "IntermediateType must have same signedness as BaseType");

    // For introspection using `decltype(fixed<...>)`
    using base_type = BaseType;
    using intermediate_type = IntermediateType;
    static constexpr decltype(FractionBits) fraction_bits = FractionBits;
    static constexpr decltype(FractionBits) integral_bits = (sizeof(BaseType) * 8) - FractionBits;
    static constexpr decltype(EnableRounding) enable_rounding = EnableRounding;

    /// Although this value fits in the BaseType in terms of bits, if there's only one integral bit, this value
    /// is incorrect (flips from positive to negative), so we must extend the size to IntermediateType.
    static constexpr IntermediateType FRACTION_MULT = IntermediateType(1) << FractionBits;

#pragma region Constructors

private:
    struct raw_construct_tag {};
    constexpr inline fixed(BaseType val, raw_construct_tag) noexcept : m_value(val) {}

public:
    constexpr inline fixed() noexcept = default;

    /// Converts an integral number to the fixed-point type.
    /// Like static_cast, this truncates bits that don't fit.
    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    constexpr inline explicit fixed(T val) noexcept
        : m_value(static_cast<BaseType>(val * FRACTION_MULT))
    {}

    /// Converts a floating-point number to the fixed-point type.
    /// Like static_cast, this truncates bits that don't fit.
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    constexpr inline explicit fixed(T val) noexcept
        : m_value(
            static_cast<BaseType>((EnableRounding) ?
            (val >= 0.0) ? (val * FRACTION_MULT + T{0.5}) : (val * FRACTION_MULT - T{0.5}) :
            (val * FRACTION_MULT))
        )
    {}

    /// Constructs from another fixed-point type with possibly different underlying representation.
    /// Like static_cast, this truncates bits that don't fit.
    template <typename B, typename I, unsigned int F, bool R>
    constexpr inline explicit fixed(fixed<B,I,F,R> val) noexcept
        : m_value(from_fixed_point<F>(val.raw_value()).raw_value())
    {}

#pragma endregion

#pragma region Conversion Operators

    /// Explicit conversion to a floating-point type
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    FPM_NODISCARD constexpr inline explicit operator T() const noexcept
    {
        return static_cast<T>(m_value) / FRACTION_MULT;
    }

    /// Explicit conversion to an integral type
    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    FPM_NODISCARD constexpr inline explicit operator T() const noexcept
    {
        return static_cast<T>(m_value / FRACTION_MULT);
    }

#pragma region fpm::fixed conversion

    /// Change number of Fraction Bits.
    template <typename I, unsigned int F, bool R>
    FPM_NODISCARD constexpr inline explicit operator fixed<BaseType, I, F, R>() const noexcept
    {
        static_assert(F != FractionBits);
        if(F > FractionBits) // Target has more bits
        {
            return fixed<BaseType, I, F, R>::from_raw_value(m_value << (F - FractionBits));
        }
        else // F < FractionBits // Target has less bits
        {
            return fixed<BaseType, I, F, R>::from_raw_value(m_value >> (FractionBits - F));
        }
    }

    /// Change Base Type.
    template <typename B, typename I, bool R>
    FPM_NODISCARD constexpr inline explicit operator fixed<B, I, FractionBits, R>() const noexcept
    {
        static_assert(sizeof(B) != sizeof(BaseType));
        return fixed<B, I, FractionBits, R>::from_raw_value(static_cast<B>(m_value));
    }

    /// Change Base Type and Fraction Bits.
    template <typename B, typename I, unsigned int F, bool R>
    FPM_NODISCARD constexpr inline explicit operator fixed<B, I, F, R>() const noexcept
    {
        static_assert(F != FractionBits);
        static_assert(sizeof(B) != sizeof(BaseType));
        if(sizeof(B) > sizeof(BaseType)) // New type is greater
        {
            const auto f = static_cast<fixed<B, I, FractionBits, R>>(*this);
            return static_cast<fixed<B, I, F, R>>(f);;
        }
        else // sizeof(B) < sizeof(BaseType) // Old type is greater
        {
            const auto f = static_cast<fixed<BaseType, I, F, R>>(*this);
            return static_cast<fixed<B, I, F, R>>(f);;
        }
    }

#pragma endregion

#pragma endregion

#pragma region Raw Values

    /// Returns the raw underlying value of this type.
    /// Do not use this unless you know what you're doing.
    FPM_NODISCARD constexpr inline BaseType raw_value() const noexcept
    {
        return m_value;
    }

    //! Constructs a fixed-point number from another fixed-point number.
    //! \tparam NumFractionBits the number of bits used by the fraction in \a value.
    //! \param value the integer fixed-point number
    template <unsigned int NumFractionBits, typename T, typename std::enable_if<(NumFractionBits > FractionBits)>::type* = nullptr>
    FPM_NODISCARD static constexpr inline fixed from_fixed_point(T value) noexcept
    {
        // To correctly round the last bit in the result, we need one more bit of information.
        // We do this by multiplying by two before dividing and adding the LSB to the real result.
        return (EnableRounding) ? fixed(static_cast<BaseType>(
             value / (T(1) << (NumFractionBits - FractionBits)) +
            (value / (T(1) << (NumFractionBits - FractionBits - 1)) % 2)),
            raw_construct_tag{}) :
            fixed(static_cast<BaseType>(value / (T(1) << (NumFractionBits - FractionBits))),
             raw_construct_tag{});
    }

    template <unsigned int NumFractionBits, typename T, typename std::enable_if<(NumFractionBits <= FractionBits)>::type* = nullptr>
    FPM_NODISCARD static constexpr inline fixed from_fixed_point(T value) noexcept
    {
        return fixed(static_cast<BaseType>(
            value * (T(1) << (FractionBits - NumFractionBits))),
            raw_construct_tag{});
    }

    /// Constructs a fixed-point number from its raw underlying value.
    /// Do not use this unless you know what you're doing.
    FPM_NODISCARD static constexpr inline fixed from_raw_value(BaseType value) noexcept
    {
        return fixed(value, raw_construct_tag{});
    }

#pragma endregion

#pragma region Constants

    FPM_NODISCARD static constexpr fixed e() { return from_fixed_point<61>(6267931151224907085ll); }
    FPM_NODISCARD static constexpr fixed pi() { return from_fixed_point<61>(7244019458077122842ll); }
    FPM_NODISCARD static constexpr fixed half_pi() { return from_fixed_point<62>(7244019458077122842ll); }
    FPM_NODISCARD static constexpr fixed two_pi() { return from_fixed_point<60>(7244019458077122842ll); }

#pragma endregion

    FPM_NODISCARD constexpr inline explicit operator bool() const noexcept
    {
        return m_value != 0;
    }

#pragma region Arithmetic member operators

#pragma region Addition

    inline fixed& operator+=(const fixed& y) noexcept
    {
        m_value += y.m_value;
        return *this;
    }

    template <typename I, typename std::enable_if<std::is_integral<I>::value>::type* = nullptr>
    inline fixed& operator+=(I y) noexcept
    {
        m_value += y * FRACTION_MULT;
        return *this;
    }

#pragma endregion

#pragma region Subtraction

    inline fixed& operator-=(const fixed& y) noexcept
    {
        m_value -= y.m_value;
        return *this;
    }

    template <typename I, typename std::enable_if<std::is_integral<I>::value>::type* = nullptr>
    inline fixed& operator-=(I y) noexcept
    {
        m_value -= y * FRACTION_MULT;
        return *this;
    }

#pragma endregion

#pragma region Multiplication

    inline fixed& operator*=(const fixed& y) noexcept
    {
        if (EnableRounding){
            // Normal fixed-point multiplication is: x * y / 2**FractionBits.
            // To correctly round the last bit in the result, we need one more bit of information.
            // We do this by multiplying by two before dividing and adding the LSB to the real result.
            auto value = (static_cast<IntermediateType>(m_value) * y.m_value) / (FRACTION_MULT / 2);
            m_value = static_cast<BaseType>((value / 2) + (value % 2));
        } else {
            auto value = (static_cast<IntermediateType>(m_value) * y.m_value) / FRACTION_MULT;
            m_value = static_cast<BaseType>(value);
        }
        return *this;
    }

    template <typename I, typename std::enable_if<std::is_integral<I>::value>::type* = nullptr>
    inline fixed& operator*=(I y) noexcept
    {
        m_value *= y;
        return *this;
    }

#pragma endregion

#pragma region Division

    inline fixed& operator/=(const fixed& y) noexcept
    {
        assert(y.m_value != 0);
        if (EnableRounding){
            // Normal fixed-point division is: x * 2**FractionBits / y.
            // To correctly round the last bit in the result, we need one more bit of information.
            // We do this by multiplying by two before dividing and adding the LSB to the real result.
            auto value = (static_cast<IntermediateType>(m_value) * FRACTION_MULT * 2) / y.m_value;
            m_value = static_cast<BaseType>((value / 2) + (value % 2));
        } else {
            auto value = (static_cast<IntermediateType>(m_value) * FRACTION_MULT) / y.m_value;
            m_value = static_cast<BaseType>(value);
        }
        return *this;
    }

    template <typename I, typename std::enable_if<std::is_integral<I>::value>::type* = nullptr>
    inline fixed& operator/=(I y) noexcept
    {
        m_value /= y;
        return *this;
    }

#pragma endregion

#pragma endregion

private:
    BaseType m_value;
};

#pragma region Convenience typedefs

using fixed_8_8 = fixed<std::int16_t, std::int32_t, 8>;

using fixed_16_16 = fixed<std::int32_t, std::int64_t, 16>;
using fixed_24_8 = fixed<std::int32_t, std::int64_t, 8>;
using fixed_8_24 = fixed<std::int32_t, std::int64_t, 24>;

#pragma endregion

template <typename B, typename I, unsigned int F, bool R, typename std::enable_if<std::is_signed<B>::value>::type* = nullptr>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator-(const fixed<B, I, F, R>& x) noexcept
{
    return fixed<B, I, F, R>::from_raw_value(-x.raw_value());
}

#pragma region Arithmetic operators

#pragma region Addition

template <typename B, typename I, unsigned int F, bool R>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator+(const fixed<B, I, F, R>& x, const fixed<B, I, F, R>& y) noexcept
{
    return fixed<B, I, F, R>::from_raw_value(x.raw_value() + y.raw_value());
}

template <typename B, typename I, unsigned int F, bool R, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator+(const fixed<B, I, F, R>& x, T y) noexcept
{
    return x + fixed<B, I, F, R>(y);
}

template <typename B, typename I, unsigned int F, bool R, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator+(T x, const fixed<B, I, F, R>& y) noexcept
{
    return fixed<B, I, F, R>(x) + y;
}

#pragma endregion

#pragma region Subtraction

template <typename B, typename I, unsigned int F, bool R>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator-(const fixed<B, I, F, R>& x, const fixed<B, I, F, R>& y) noexcept
{
    return fixed<B, I, F, R>::from_raw_value(x.raw_value() - y.raw_value());
}

template <typename B, typename I, unsigned int F, bool R, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator-(const fixed<B, I, F, R>& x, T y) noexcept
{
    return x - fixed<B, I, F, R>(y);
}

template <typename B, typename I, unsigned int F, bool R, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator-(T x, const fixed<B, I, F, R>& y) noexcept
{
    return fixed<B, I, F, R>(x) - y;
}

#pragma endregion

#pragma region Multiplication

template <typename B, typename I, unsigned int F, bool R>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator*(const fixed<B, I, F, R>& x, const fixed<B, I, F, R>& y) noexcept
{
    if (R){
        // Normal fixed-point multiplication is: x * y / 2**FractionBits.
        // To correctly round the last bit in the result, we need one more bit of information.
        // We do this by multiplying by two before dividing and adding the LSB to the real result.
        const auto value = (static_cast<I>(x.raw_value()) * y.raw_value()) / (fixed<B, I, F, R>::FRACTION_MULT / 2);
        return fixed<B, I, F, R>::from_raw_value(static_cast<B>((value / 2) + (value % 2)));
    } else {
        const auto value = (static_cast<I>(x.raw_value()) * y.raw_value()) / fixed<B, I, F, R>::FRACTION_MULT;
        return fixed<B, I, F, R>::from_raw_value(static_cast<B>(value));
    }
}

template <typename B, typename I, unsigned int F, bool R, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator*(const fixed<B, I, F, R>& x, T y) noexcept
{
    return fixed<B, I, F, R>::from_raw_value(x.raw_value() * y);
}

template <typename B, typename I, unsigned int F, bool R, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator*(T x, const fixed<B, I, F, R>& y) noexcept
{
    return fixed<B, I, F, R>::from_raw_value(x * y.raw_value());
}
#pragma endregion

#pragma region Division

template <typename B, typename I, unsigned int F, bool R>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator/(const fixed<B, I, F, R>& x, const fixed<B, I, F, R>& y) noexcept
{
    assert(y.raw_value() != 0);
    if (R){
        // Normal fixed-point division is: x * 2**FractionBits / y.
        // To correctly round the last bit in the result, we need one more bit of information.
        // We do this by multiplying by two before dividing and adding the LSB to the real result.
        const auto value = (static_cast<I>(x.raw_value()) * fixed<B, I, F, R>::FRACTION_MULT * 2) / y.raw_value();
        return fixed<B, I, F, R>::from_raw_value(static_cast<B>((value / 2) + (value % 2)));
    } else {
        const auto value = (static_cast<I>(x.raw_value()) * fixed<B, I, F, R>::FRACTION_MULT) / y.raw_value();
        return fixed<B, I, F, R>::from_raw_value(static_cast<B>(value));
    }
}

template <typename B, typename I, unsigned int F, typename T, bool R, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator/(const fixed<B, I, F, R>& x, T y) noexcept
{
    return fixed<B, I, F, R>::from_raw_value(x.raw_value() / y);
}

template <typename B, typename I, unsigned int F, typename T, bool R, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
FPM_NODISCARD constexpr inline fixed<B, I, F, R> operator/(T x, const fixed<B, I, F, R>& y) noexcept
{
    return fixed<B, I, F, R>(x) / y;
}

#pragma endregion

#pragma endregion

#pragma region Comparison operators

template <typename B, typename I, unsigned int F, bool R>
FPM_NODISCARD constexpr inline bool operator==(const fixed<B, I, F, R>& x, const fixed<B, I, F, R>& y) noexcept
{
    return x.raw_value() == y.raw_value();
}

template <typename B, typename I, unsigned int F, typename T, bool R, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
FPM_NODISCARD constexpr inline bool operator==(const fixed<B, I, F, R>& x, const T y) noexcept
{
    return x == fixed<B, I, F, R>{y};
}

template <typename B, typename I, unsigned int F, typename T, bool R, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
FPM_NODISCARD constexpr inline bool operator==(const T x, const fixed<B, I, F, R>& y) noexcept
{
    return fixed<B, I, F, R>{x} == y;
}

#if __cplusplus >= 202002L /* C++20 */

template <typename B, typename I, unsigned int F, bool R>
FPM_NODISCARD constexpr inline auto operator<=>(const fixed<B, I, F, R>& x, const fixed<B, I, F, R>& y) noexcept
{
    return x.raw_value() <=> y.raw_value();
}

#else

template <typename B, typename I, unsigned int F, bool R>
FPM_NODISCARD constexpr inline bool operator!=(const fixed<B, I, F, R>& x, const fixed<B, I, F, R>& y) noexcept
{
    return x.raw_value() != y.raw_value();
}

template <typename B, typename I, unsigned int F, bool R>
FPM_NODISCARD constexpr inline bool operator<(const fixed<B, I, F, R>& x, const fixed<B, I, F, R>& y) noexcept
{
    return x.raw_value() < y.raw_value();
}

template <typename B, typename I, unsigned int F, bool R>
FPM_NODISCARD constexpr inline bool operator>(const fixed<B, I, F, R>& x, const fixed<B, I, F, R>& y) noexcept
{
    return x.raw_value() > y.raw_value();
}

template <typename B, typename I, unsigned int F, bool R>
FPM_NODISCARD constexpr inline bool operator<=(const fixed<B, I, F, R>& x, const fixed<B, I, F, R>& y) noexcept
{
    return x.raw_value() <= y.raw_value();
}

template <typename B, typename I, unsigned int F, bool R>
FPM_NODISCARD constexpr inline bool operator>=(const fixed<B, I, F, R>& x, const fixed<B, I, F, R>& y) noexcept
{
    return x.raw_value() >= y.raw_value();
}

#endif
#pragma endregion

namespace detail
{
/// Number of base-10 digits required to fully represent a number of bits.
FPM_NODISCARD static constexpr inline int max_digits10(int bits) noexcept
{
    // 8.24 fixed-point equivalent of (int)ceil(bits * std::log10(2));
    using T = long long;
    return static_cast<int>((T{bits} * 5050445 + (T{1} << 24) - 1) >> 24);
}

/// Number of base-10 digits that can be fully represented by a number of bits.
FPM_NODISCARD static constexpr inline int digits10(int bits) noexcept
{
    // 8.24 fixed-point equivalent of (int)(bits * std::log10(2));
    using T = long long;
    return static_cast<int>((T{bits} * 5050445) >> 24);
}

} // namespace detail
} // namespace fpm

// Specializations for customization points
namespace std
{

template <typename B, typename I, unsigned int F, bool R>
struct hash<fpm::fixed<B,I,F,R>>
{
    using argument_type = fpm::fixed<B, I, F, R>;
    using result_type = std::size_t;

    FPM_NODISCARD result_type operator()(argument_type arg) const noexcept(noexcept(std::declval<std::hash<B>>()(arg.raw_value()))) {
        return std::hash<B>{}(arg.raw_value());
    }
};

#pragma region numeric_limits

template <typename B, typename I, unsigned int F, bool R>
struct numeric_limits<fpm::fixed<B,I,F,R>>
{
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = std::numeric_limits<B>::is_signed;
    static constexpr bool is_integer = false;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr std::float_denorm_style has_denorm = std::denorm_absent;
    static constexpr bool has_denorm_loss = false;
    static constexpr std::float_round_style round_style = std::round_to_nearest;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = std::numeric_limits<B>::is_modulo;
    static constexpr int digits = std::numeric_limits<B>::digits;

    // Any number with `digits10` significant base-10 digits (that fits in
    // the range of the type) is guaranteed to be convertible from text and
    // back without change. Worst case, this is 0.000...001, so we can only
    // guarantee this case. Nothing more.
    static constexpr int digits10 = 1;

    // This is equal to max_digits10 for the integer and fractional part together.
    static constexpr int max_digits10 =
        fpm::detail::max_digits10(std::numeric_limits<B>::digits - F) + fpm::detail::max_digits10(F);

    static constexpr int radix = 2;
    static constexpr int min_exponent = 1 - F;
    static constexpr int min_exponent10 = -fpm::detail::digits10(F);
    static constexpr int max_exponent = std::numeric_limits<B>::digits - F;
    static constexpr int max_exponent10 = fpm::detail::digits10(std::numeric_limits<B>::digits - F);
    static constexpr bool traps = true;
    static constexpr bool tinyness_before = false;

    static constexpr fpm::fixed<B,I,F,R> lowest() noexcept {
        return fpm::fixed<B,I,F,R>::from_raw_value(std::numeric_limits<B>::lowest());
    };

    static constexpr fpm::fixed<B,I,F,R> min() noexcept {
        return lowest();
    }

    static constexpr fpm::fixed<B,I,F,R> max() noexcept {
        return fpm::fixed<B,I,F,R>::from_raw_value(std::numeric_limits<B>::max());
    };

    static constexpr fpm::fixed<B,I,F,R> epsilon() noexcept {
        return fpm::fixed<B,I,F,R>::from_raw_value(1);
    };

    static constexpr fpm::fixed<B,I,F,R> round_error() noexcept {
        return fpm::fixed<B,I,F,R>(1) / 2;
    };

    static constexpr fpm::fixed<B,I,F,R> denorm_min() noexcept {
        return min();
    }
};

template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::is_specialized;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::is_signed;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::is_integer;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::is_exact;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::has_infinity;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::has_quiet_NaN;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::has_signaling_NaN;
template <typename B, typename I, unsigned int F, bool R>
constexpr std::float_denorm_style numeric_limits<fpm::fixed<B,I,F,R>>::has_denorm;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::has_denorm_loss;
template <typename B, typename I, unsigned int F, bool R>
constexpr std::float_round_style numeric_limits<fpm::fixed<B,I,F,R>>::round_style;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::is_iec559;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::is_bounded;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::is_modulo;
template <typename B, typename I, unsigned int F, bool R>
constexpr int numeric_limits<fpm::fixed<B,I,F,R>>::digits;
template <typename B, typename I, unsigned int F, bool R>
constexpr int numeric_limits<fpm::fixed<B,I,F,R>>::digits10;
template <typename B, typename I, unsigned int F, bool R>
constexpr int numeric_limits<fpm::fixed<B,I,F,R>>::max_digits10;
template <typename B, typename I, unsigned int F, bool R>
constexpr int numeric_limits<fpm::fixed<B,I,F,R>>::radix;
template <typename B, typename I, unsigned int F, bool R>
constexpr int numeric_limits<fpm::fixed<B,I,F,R>>::min_exponent;
template <typename B, typename I, unsigned int F, bool R>
constexpr int numeric_limits<fpm::fixed<B,I,F,R>>::min_exponent10;
template <typename B, typename I, unsigned int F, bool R>
constexpr int numeric_limits<fpm::fixed<B,I,F,R>>::max_exponent;
template <typename B, typename I, unsigned int F, bool R>
constexpr int numeric_limits<fpm::fixed<B,I,F,R>>::max_exponent10;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::traps;
template <typename B, typename I, unsigned int F, bool R>
constexpr bool numeric_limits<fpm::fixed<B,I,F,R>>::tinyness_before;

#pragma endregion

}

// Type testing
namespace fpm
{
template<typename T>
struct is_fixed : std::false_type {};

template<typename BaseType, typename IntermediateType, unsigned int FractionBits, bool EnableRounding>
struct is_fixed<fixed<BaseType, IntermediateType, FractionBits, EnableRounding>> : std::true_type {};

#if  __cplusplus >= 201703L /* C++17 */
template<typename T>
inline constexpr bool is_fixed_v = is_fixed<T>::value;
#endif
}
#endif
