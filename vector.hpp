#ifndef VECTORHPP
#define VECTORHPP
// 3D vector of any arithmetic type
template <typename T> struct Vec3D {
    // Compile-time assert statement to ensure that T is something that allows math operations
    static_assert(std::is_arithmetic_v<T>, "T is not an arithmetic type");
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
    // Addition
    Vec3D operator+(Vec3D const &v) const {
        return {x + v.x, y + v.y, z + v.z};
    }
    // Scalar multiplication
    Vec3D operator*(const T s) const {
        return {x * s, y * s, z * s};
    }
    // Scalar multiplicative assignment
    void operator*=(const T s) {
        x *= s; y *= s; z *= s;
    }
    // Multiplication by a scalar
    friend Vec3D operator*(T s, const Vec3D& v) {
        return {v.x * s, v.y * s, v.z * s};
    }
    // Dot product
    T operator*(Vec3D const &v) const {
        return x * v.x + y * v.y + z * v.z;
    }
    // Cross Product
    Vec3D operator^(Vec3D const &v) const {
        return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
    }
    // Cross product assignment
    void operator^=(Vec3D const v) {
        double xp = y * v.z - z * v.y;
        double yp = z * v.x - x * v.z;
        double zp = x * v.y - y * v.x;
        x = xp; y = yp; z = zp;
    }
    // Vector addition
    Vec3D operator+(Vec3D const &v) {
        return {x + v.x, y + v.y, z + v.z};
    }
    // Vector additive assignment
    void operator+=(Vec3D const &v) {
        x += v.x; y += v.y; z += v.z;
    }
    // Vector subtraction
    Vec3D operator-(Vec3D const &v) const {
        return {x - v.x, y - v.y, z - v.z};
    }
    // Vector subtractive assignment
    void operator-=(Vec3D const &v) {
        x -= v.x; y -= v.y; z -= v.z;
    }
    // Unary negation
    Vec3D operator-() const {
        return Vec3D(-x, -y, -z);
    }
    // Squared magnitude
    // Exists incase we end up using this for gravity math, where it's slightly optimal to do mag() * magSquared()
    T magSquared() const {
        return x * x + y * y + z * z;
    }
    // Magnitude (length)
    T mag() const {
        // Promotes the type to a double
        using FloatingType = std::common_type_t<T, double>;
        // Since it's so wordy:
        // val = sqrt(magSquared)
        FloatingType val = std::sqrt(static_cast<FloatingType>(magSquared()));
        return static_cast<T>(val);
    }
    // Normalize operator
    Vec3D norm() const {
        const T length = mag();
        // Fallback incase it's already zero
        if (length == 0) return {0, 0, 0};
        // Promotion to double
        using FloatingType = std::common_type_t<T, double>;
        FloatingType invMag = 1.0 / static_cast<FloatingType>(length);
        // Cast it back to T after we use double precision
        return {
            static_cast<T>(x * invMag),
            static_cast<T>(y * invMag),
            static_cast<T>(z * invMag)
        };
    }
    // Scalar comparison: greater than
    // Equivalent to v.mag() > m
    bool operator>(const T m) const {
        return mag() > m;
    }
    // Scalar comparison: greater than or equal to
    // Equivalent to v.mag() >= m
    bool operator>=(const T m) const {
        return mag() >= m;
    }
    // Scalar comparison: less than
    // Equivalent to v.mag() < m
    bool operator<(const T m) const {
        return mag() < m;
    }
    // Scalar comparison: less than or equal to
    // Equivalent to v.mag() <= m
    bool operator<=(const T m) const {
        return mag() <= m;
    }
    // Scalar division
    Vec3D operator/(const T m) const {
        using FloatingType = std::common_type_t<T, double>;
        FloatingType id = 1.0 / static_cast<FloatingType>(m);
        return {
            static_cast<T>(x * id),
            static_cast<T>(y * id),
            static_cast<T>(z * id)
        };
    }
    // Scalar division assignment
    void operator/=(const T m) {
        using FloatingType = std::common_type_t<T, double>;
        FloatingType id = 1.0 / static_cast<FloatingType>(m);
        x = static_cast<T>(x * id);
        y = static_cast<T>(y * id);
        z = static_cast<T>(z * id);
    }
    // Distance squared
    T distSquared(Vec3D const v) const {
        return (this - v).magSquared();
    }
    // Distance
    T dist(Vec3D const &v) const {
        using FloatingType = std::common_type_t<T, double>;
        FloatingType d = std::sqrt(static_cast<FloatingType>(distSquared(v)));
        return static_cast<T>(d);
    }
    // Equality
    // Warning: floating point precision applies here
    bool operator==(Vec3D const &v) const {
        return x == v.x && y == v.y && z == v.z;
    }
    // Inequality
    // Warning: floating point precision applies here
    bool operator!=(Vec3D const &v) const {
        return !(*this == v);
    }
};
#endif