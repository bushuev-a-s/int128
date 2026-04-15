#include "Int128.hpp"
#include <algorithm>

Int128::Int128(): left(), right() {}
Int128::Int128(int64_t a): left(), right(a) {}

Int128::Int128(std::string_view a) {
    bool isNegative = false;
    std::size_t i = 0;

    if (a[0] == '-') {
        isNegative = true;
        i = 1;
    }

    for (; i < a.size(); ++i) {
        int digit = a[i] - '0';
        *this = (*this * 10) + static_cast<int64_t>(digit);
    }

    if (isNegative) {
        *this = -(*this);
    }
}

Int128::operator int64_t() const {
    if (left < 0) {
        return -right;
    }
    return right;
}

Int128::operator double() const {
    const double twoPow64 = 18446744073709551616.0;

    return static_cast<double>(left) * twoPow64 + static_cast<double>(right);
}

std::string Int128::str() const {
    if (left == 0 && right == 0) return "0";

    bool isNegative = (left < 0);
    uint64_t l = static_cast<uint64_t>(left);
    uint64_t r = right;

    if (isNegative) {
        l = ~l;
        r = ~r;
        r += 1;
        if (r == 0) l += 1;
    }

    std::string s;
    while (l > 0 || r > 0) {
        uint64_t rem = 0;
        uint64_t next_l = l / 10;
        rem = l % 10;
        
        uint64_t next_r = 0;
        for (int i = 63; i >= 0; --i) {
            rem = (rem << 1) | ((r >> i) & 1);
            if (rem >= 10) {
                rem -= 10;
                next_r |= (1ULL << i);
            }
        }

        s += (char)(rem + '0');
        l = next_l;
        r = next_r;
    }

    if (isNegative) s += '-';
    std::reverse(s.begin(), s.end());
    return s;
}


Int128 Int128::operator+(const Int128& other) const {
    Int128 result;
    result.right = this->right + other.right;
    uint64_t carry = (result.right < this->right) ? 1 : 0;
    result.left = this->left + other.left + static_cast<int64_t>(carry);
    return result;
}

Int128 Int128::operator-(const Int128& other) const {
    return *this + (-other);
}

Int128 Int128::operator*(const Int128& other) const {
    bool resultNegative = (this->left < 0) != (other.left < 0);

    Int128 a = (this->left < 0) ? -(*this) : *this;
    Int128 b = (other.left < 0) ? -(other) : other;

    uint64_t a3 = (uint64_t)a.left >> 32;
    uint64_t a2 = (uint64_t)a.left & 0xFFFFFFFF;
    uint64_t a1 = a.right >> 32;
    uint64_t a0 = a.right & 0xFFFFFFFF;

    uint64_t b3 = (uint64_t)b.left >> 32;
    uint64_t b2 = (uint64_t)b.left & 0xFFFFFFFF;
    uint64_t b1 = b.right >> 32;
    uint64_t b0 = b.right & 0xFFFFFFFF;

    Int128 res(0);

    uint64_t p0 = a0 * b0;
    res.right = p0;

    auto add_part = [&](uint64_t val, int shift_bits) {
        Int128 part;
        if (shift_bits < 64) {
            part.right = val << shift_bits;
            part.left = val >> (64 - shift_bits);
        } else {
            part.right = 0;
            part.left = val << (shift_bits - 64);
        }
        res = res + part;
    };

    add_part(a0 * b1, 32);
    add_part(a1 * b0, 32);
    add_part(a0 * b2, 64);
    add_part(a1 * b1, 64);
    add_part(a2 * b0, 64);
    add_part(a0 * b3, 96);
    add_part(a1 * b2, 96);
    add_part(a2 * b1, 96);
    add_part(a3 * b0, 96);

    return resultNegative ? -res : res;
}

Int128 Int128::operator/(const Int128& other) const {
    bool resultNeg = (this->left < 0) != (other.left < 0);

    // 3. Берем модули (положительные значения)
    Int128 dnd = (this->left < 0) ? -(*this) : *this; // Делимое (dividend)
    Int128 dsr = (other.left < 0) ? -other : other;   // Делитель (divisor)

    Int128 quot; // Частное (результат)
    quot.right = 0; quot.left = 0;
    
    Int128 rem;  // Текущий остаток
    rem.right = 0; rem.left = 0;

    // 4. Побитовый цикл (идем от самого старшего бита к младшему)
    for (int i = 127; i >= 0; --i) {
        // --- Сдвигаем остаток влево на 1 бит ---
        uint64_t carry_rem = (rem.right >> 63) & 1;
        rem.right <<= 1;
        rem.left = (static_cast<uint64_t>(rem.left) << 1) | carry_rem;

        // --- Втягиваем i-й бит делимого в конец остатка ---
        uint64_t bit;
        if (i >= 64) bit = (static_cast<uint64_t>(dnd.left) >> (i - 64)) & 1;
        else         bit = (dnd.right >> i) & 1;
        
        rem.right |= bit;

        // --- Сравнение: rem >= dsr ---
        bool gte = false;
        if (static_cast<uint64_t>(rem.left) > static_cast<uint64_t>(dsr.left)) {
            gte = true;
        } else if (static_cast<uint64_t>(rem.left) == static_cast<uint64_t>(dsr.left)) {
            if (rem.right >= dsr.right) gte = true;
        }

        // --- Если остаток больше делителя, вычитаем и ставим бит в частном ---
        if (gte) {
            // Вычитание rem = rem - dsr
            uint64_t old_l = rem.right;
            rem.right -= dsr.right;
            uint64_t borrow = (rem.right > old_l) ? 1 : 0;
            rem.left -= (dsr.left + borrow);

            // Установка i-го бита в частном
            if (i >= 64) quot.left |= (1LL << (i - 64));
            else         quot.right  |= (1ULL << i);
        }
    }

    return resultNeg ? -quot : quot;
}

Int128 Int128::operator-() const {
    uint64_t l = ~static_cast<uint64_t>(this->left);
    uint64_t r = ~this->right;
    r += 1;

    if (r == 0) {
        l += 1;
    }

    Int128 result;
    result.left = static_cast<int64_t>(l);
    result.right = r;
    
    return result;
}

bool Int128::operator==(const Int128& other) const {
    if (left == other.left && right == other.right) {
        return true;
    }
    return false;
}

bool Int128::operator!=(const Int128& other) const {
    return !(*this == other);
}

Int128& Int128::operator+=(const Int128& other) {
    *this = *this + other;
    return *this;
}

Int128& Int128::operator-=(const Int128& other) {
    *this = *this - other;
    return *this;
}

Int128& Int128::operator*=(const Int128& other) {
    *this = *this * other;
    return *this;
}

Int128& Int128::operator/=(const Int128& other) {
    *this = *this / other;
    return *this;
}


std::ostream& operator<<(std::ostream& os, const Int128& value) {
    return os << value.str();
}
