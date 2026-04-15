#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <cstdint>

class Int128
{
public:
    Int128();
    Int128(int64_t a);
    Int128(std::string_view a);

    explicit operator int64_t() const;
    explicit operator double() const;
    std::string str() const;

    Int128 operator+(const Int128& other) const;
    Int128 operator-(const Int128& other) const;
    Int128 operator*(const Int128& other) const;
    Int128 operator/(const Int128& other) const;

    Int128 operator-() const;

    bool operator==(const Int128& other) const;
    bool operator!=(const Int128& other) const;

    Int128& operator+=(const Int128& other);
    Int128& operator-=(const Int128& other);
    Int128& operator*=(const Int128& other);
    Int128& operator/=(const Int128& other);

    friend std::ostream& operator<<(std::ostream& os, const Int128& value);
private:
    int64_t left = 0;
    uint64_t right = 0;
};
