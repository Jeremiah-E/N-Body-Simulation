#ifndef VECTORHPP
#define VECTORHPP
// 3D vector of any arithmetic type
template <typename T> struct Vec3D {
    // Compile-time assert statement to ensure that T is something that allows math operations
    static_assert(is_arithmetic_v<T>, "T is not an arithmetic type");
    // Any time a floating type is needed, use F instead of T
    using F = common_type_t<T, double>;

    // The components of the vector. Only makes sense when in some reference frame, which will be derived from the dataset
    T x; T y; T z;

    // Default constructor, all zeroes
    Vec3D() : x(0), y(0), z(0) {};
    // Constructor given T [3]
    Vec3D(const T var[3]) : x(var[0]), y(var[1]), z(var[2]) {}
    // Constructor given three literals
    Vec3D(T px, T py, T pz) : x(px), y(py), z(pz) {}
    // Constructor given T *
    // Ensure the pointer has enough room to call var[2]
    Vec3D(T *var) : Vec3D(var[0], var[1], var[2]){}

    // Scalar multiplication
    // v * s
    Vec3D operator*(const T s) const {
        return {x * s, y * s, z * s};
    }
    // Scalar multiplicative assignment
    // v *= s
    void operator*=(const T s) {
        x *= s; y *= s; z *= s;
    }
    // Multiplication by a scalar
    // s * v
    friend Vec3D operator*(const T s, const Vec3D& v) {
        return {v.x * s, v.y * s, v.z * s};
    }
    // Dot product
    // v * u
    T operator*(Vec3D const &u) const {
        return x * u.x + y * u.y + z * u.z;
    }
    // Cross Product
    // v ^ u
    Vec3D operator^(Vec3D const &u) const {
        return {y * u.z - z * u.y, z * u.x - x * u.z, x * u.y - y * u.x};
    }
    // Cross product assignment
    // v ^= u
    void operator^=(Vec3D const u) {
        const double xp = y * u.z - z * u.y;
        const double yp = z * u.x - x * u.z;
        const double zp = x * u.y - y * u.x;
        x = xp; y = yp; z = zp;
    }
    // Scalar division
    // v / s
    Vec3D operator/(const T s) const {
        const F invS = 1.0 / static_cast<F>(s);
        return {
            static_cast<T>(x * invS),
            static_cast<T>(y * invS),
            static_cast<T>(z * invS)
        };
    }
    // Scalar division assignment
    // v /= s
    void operator/=(const T s) {
        const F invS = 1.0 / static_cast<F>(s);
        x = static_cast<T>(x * invS);
        y = static_cast<T>(y * invS);
        z = static_cast<T>(z * invS);
    }
    // Vector addition
    // v + u
    Vec3D operator+(Vec3D const &u) const {
        return {x + u.x, y + u.y, z + u.z};
    }
    // Vector additive assignment
    // v += u
    void operator+=(Vec3D const &u) {
        x += u.x; y += u.y; z += u.z;
    }
    // Vector subtraction
    // v - u
    Vec3D operator-(Vec3D const &u) const {
        return {x - u.x, y - u.y, z - u.z};
    }
    // Vector subtractive assignment
    // v -= u
    void operator-=(Vec3D const &u) {
        x -= u.x; y -= u.y; z -= u.z;
    }
    // Unary negation
    // -v
    Vec3D operator-() const {
        return Vec3D(-x, -y, -z);
    }
    // Squared magnitude (length squared)
    // Cheaper to compute than mag()
    // v.magSquared()
    T magSquared() const {
        return x * x + y * y + z * z;
    }
    // Magnitude (length)
    // v.mag()
    T mag() const {
        F val = sqrt(static_cast<F>(magSquared()));
        return static_cast<T>(val);
    }
    // Normalize operator, v.norm() == v/|v|
    // v.norm()
    Vec3D norm() const {
        const T length = mag();
        // Fallback incase it's already zero
        if (length == 0) { return Vec3D<T>(); }
        const F invMag = 1.0 / static_cast<F>(length);
        // Cast it back to T after we use double precision
        return {
            static_cast<T>(x * invMag),
            static_cast<T>(y * invMag),
            static_cast<T>(z * invMag)
        };
    }
    // Scalar comparison: greater than
    // Equivalent to v.mag() > s
    // v > s
    bool operator>(const T s) const {
        return magSquared() > sqrt(s);
    }
    // Scalar comparison: greater than or equal to
    // Equivalent to v.mag() >= s
    // v >= s
    bool operator>=(const T s) const {
        return magSquared() >= sqrt(s);
    }
    // Scalar comparison: less than
    // Equivalent to v.mag() < m
    // v < s
    bool operator<(const T s) const {
        return magSquared() < sqrt(s);
    }
    // Scalar comparison: less than or equal to
    // Equivalent to v.mag() <= s
    // v <= s
    bool operator<=(const T s) const {
        return magSquared() <= sqrt(s);
    }
    // Distance squared
    // v.distSquared(u)
    T distSquared(Vec3D const u) const {
        return ((*this) - u).magSquared();
    }
    // Distance
    // v.dist(u)
    T dist(Vec3D const &u) const {
        const F d = sqrt(static_cast<F>(distSquared(u)));
        return static_cast<T>(d);
    }
    // Equality
    // Warning: floating point precision applies here
    // v == u
    bool operator==(Vec3D const &u) const {
        return (x == u.x && y == u.y && z == u.z);
    }
    // Inequality
    // Warning: floating point precision applies here
    // v != u
    bool operator!=(Vec3D const &u) const {
        return (x != u.x || y != u.y || z != u.z);
    }
    // Rotate this vector by angle around axis, using Rodrigues' rotation formula
    // Assumes axis is a unit vector. Generated by Claude.
    // v.rotate(axis, angle)
    Vec3D rotate(Vec3D const &axis, const T angle) const {
        const F cosine = cos(static_cast<F>(angle));
        const F sine = sin(static_cast<F>(angle));
        const F versine = 1.0 - cosine;
        return (*this) * static_cast<T>(cosine) + (axis ^ (*this)) * static_cast<T>(sine) + axis * static_cast<T>((axis * (*this)) * versine);
    }
};
#endif