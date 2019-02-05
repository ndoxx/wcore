#ifndef MATH_STRUCTURES_H
#define MATH_STRUCTURES_H

#include <cassert>
#include <cmath>
#include <utility>
#include <iomanip>
#include <sstream>
#include <array>

namespace wcore
{
namespace math
{
namespace detail
{
    // Zero Clip: If value near 0 up to precision return 0
    inline float ZC(float value, float precision=1e-5){
        if(fabs(value)<precision)
            return 0.0;
        return value;
    }
}

/*
                     _   _           _
                    | | | |         | |
                    | | | | ___  ___| |_ ___  _ __
                    | | | |/ _ \/ __| __/ _ \| '__|
                    \ \_/ /  __/ (__| || (_) | |
                     \___/ \___|\___|\__\___/|_|
*/

template <unsigned N, typename T=float>
class vec
{
private:
    template <typename... Args>
    void push(unsigned pos, T first, Args&&... args)
    {
        if(pos==N) return;   // Supernumerary arguments
        value_[pos] = T(first);
        push(++pos, std::forward<Args>(args)...);
    }

    void push(unsigned pos)
    {
        if(pos!=N) // Not enough arguments, fill with zeros
        {
            for(;pos<N;++pos)
                value_[pos] = T(0);
        }
    }

public:
    static const unsigned Size = N;

    vec()
    {
        for(unsigned ii=0; ii<N; ++ii)
            value_[ii] = T(0);
    }
    explicit vec(std::initializer_list<T> coeffs)
    {
        uint32_t index = 0;
        for(auto it=coeffs.begin(); it!=coeffs.end() && index<N; ++it)
        {
            value_[index++] = *it;
        }
    }
    explicit vec(T unique_value)
    {
        for(unsigned ii=0; ii<N; ++ii)
            value_[ii] = unique_value;
    }
    template <typename... Args>
    explicit vec(T first, Args&&... args)
    {
        value_[0] = first;
        push(1,args...);
    }
    /*vec(const vec& right)
    {
        std::copy(right.value_,right.value_+N,value_);
    }
    vec(vec&& right)
    {
        std::swap(value_, right.value_);
    }*/
    explicit vec(const vec<N-1,T>& right)
    {
        for(unsigned ii=0; ii<N-1; ++ii)
            value_[ii] = right[ii];
        value_[N-1] = T(0);
    }
    explicit vec(const vec<N-1,T>& right, const T& last)
    {
        for(unsigned ii=0; ii<N-1; ++ii)
            value_[ii] = right[ii];
        value_[N-1] = last;
    }
    template <unsigned D>
    explicit vec(const vec<D,T>& right)
    {
        for(unsigned ii=0; ii<D; ++ii)
            value_[ii] = right[ii];
        for(unsigned ii=D; ii<N; ++ii)
            value_[ii] = T(0);
    }

    void swap(vec<N,T>& other)
    {
        std::swap(value_, other.value_);
    }

    inline std::array<T, N> to_array()
    {
        std::array<T, N> out;
        for(uint32_t ii=0; ii<N; ++ii)
            out[ii] = value_[ii];
        return out;
    }

    // Bracket accessors
    inline T& operator[](unsigned index)
    {
        assert(index<N && "vec::operator[]: index out of bound.");
        return value_[index];
    }
    inline T operator[](unsigned index) const
    {
        assert(index<N && "vec::operator[] const: index out of bound.");
        return value_[index];
    }

    // Generic accessors
    template <unsigned POS, bool U=true, typename=typename std::enable_if<U&&(POS<N)>::type>
    inline const T& Get() const { return value_[POS]; }
    template <unsigned POS, bool U=true, typename=typename std::enable_if<U&&(POS<N)>::type>
    inline void     Set(T value){ value_[POS]=value; }

    // Helper Accessors
    inline T x() const { return value_[0]; }
    inline T y() const { return value_[1]; }
    template <bool U=true, typename=typename std::enable_if<U&&(N>2)>::type>
    inline T z() const { return value_[2]; }
    template <bool U=true, typename=typename std::enable_if<U&&(N>3)>::type>
    inline T w() const { return value_[3]; }

    inline T& x() { return value_[0]; }
    inline T& y() { return value_[1]; }
    template <bool U=true, typename=typename std::enable_if<U&&(N>2)>::type>
    inline T& z() { return value_[2]; }
    template <bool U=true, typename=typename std::enable_if<U&&(N>3)>::type>
    inline T& w() { return value_[3]; }

    inline T r() const { return value_[0]; }
    inline T g() const { return value_[1]; }
    template <bool U=true, typename=typename std::enable_if<U&&(N>2)>::type>
    inline T b() const { return value_[2]; }
    template <bool U=true, typename=typename std::enable_if<U&&(N>3)>::type>
    inline T a() const { return value_[3]; }

    inline T& r() { return value_[0]; }
    inline T& g() { return value_[1]; }
    template <bool U=true, typename=typename std::enable_if<U&&(N>2)>::type>
    inline T& b() { return value_[2]; }
    template <bool U=true, typename=typename std::enable_if<U&&(N>3)>::type>
    inline T& a() { return value_[3]; }

    template <bool U=true, typename=typename std::enable_if<U&&(N==3)>::type>
    inline vec<2,T> xy() const { return vec<2,T>(x(),y()); }
    template <bool U=true, typename=typename std::enable_if<U&&(N==3)>::type>
    inline vec<2,T> xz() const { return vec<2,T>(x(),z()); }
    template <bool U=true, typename=typename std::enable_if<U&&(N==3)>::type>
    inline vec<2,T> yx() const { return vec<2,T>(y(),x()); }
    template <bool U=true, typename=typename std::enable_if<U&&(N==3)>::type>
    inline vec<2,T> yz() const { return vec<2,T>(y(),z()); }
    template <bool U=true, typename=typename std::enable_if<U&&(N==3)>::type>
    inline vec<2,T> zx() const { return vec<2,T>(z(),x()); }
    template <bool U=true, typename=typename std::enable_if<U&&(N==3)>::type>
    inline vec<2,T> zy() const { return vec<2,T>(z(),y()); }

    template <bool U=true, typename=typename std::enable_if<U&&(N==4)>::type>
    inline vec<3,T> xyz() const { return vec<3,T>(x(),y(),z()); }

    // Comparison
    inline bool operator==(const vec& right) const
    {
        for(unsigned ii=0; ii<N; ++ii)
            if((*this)[ii] != right[ii])
                return false;
        return true;
    }
    inline bool operator!=(const vec& right) const
    {
        return !(operator==(right));
    }

    // Assignment
    /*inline vec& operator=(const vec& right)
    {
        std::copy(right.value_,right.value_+N,value_);
        return *this;
    }*/

    /*inline vec& operator=(vec&& right)
    {
        std::swap(right.value_,value_);
        return *this;
    }*/

    template <unsigned M>
    inline const vec& operator=(const vec<M>& right)
    {
        unsigned min_size = std::min(M,N);
        for(unsigned ii=0; ii<min_size; ++ii)
            value_[ii] = right[ii];
        for(unsigned ii=min_size; ii<N; ++ii)
            value_[ii] = T(0);
        return *this;
    }

    inline vec& operator+=(const vec& right)
    {
        for(unsigned ii=0; ii<N; ++ii)
            (*this)[ii] += right[ii];
        return *this;
    }
    inline vec& operator-=(const vec& right)
    {
        for(unsigned ii=0; ii<N; ++ii)
            (*this)[ii] -= right[ii];
        return *this;
    }
    inline vec& operator*=(const vec& right)
    {
        for(unsigned ii=0; ii<N; ++ii)
            (*this)[ii] *= right[ii];
        return *this;
    }
    inline vec& operator/=(const vec& right)
    {
        for(unsigned ii=0; ii<N; ++ii)
            (*this)[ii] /= right[ii];
        return *this;
    }


    inline vec& operator+=(T right)
    {
        for(unsigned ii=0; ii<N; ++ii)
            (*this)[ii] += right;
        return *this;
    }
    inline vec& operator-=(T right)
    {
        for(unsigned ii=0; ii<N; ++ii)
            (*this)[ii] -= right;
        return *this;
    }
    inline vec& operator*=(T right)
    {
        for(unsigned ii=0; ii<N; ++ii)
            (*this)[ii] *= right;
        return *this;
    }
    inline vec& operator/=(T right)
    {
        for(unsigned ii=0; ii<N; ++ii)
            (*this)[ii] /= right;
        return *this;
    }

    // Arithmetics
    // Addition
    friend vec operator+(const vec& v1, const vec& v2)
    {
        vec result;
        for(unsigned ii=0; ii<N; ++ii)
            result[ii] = v1[ii] + v2[ii];
        return result;
    }
    // Subtraction
    friend vec operator-(const vec& v1, const vec& v2)
    {
        vec result;
        for(unsigned ii=0; ii<N; ++ii)
            result[ii] = v1[ii] - v2[ii];
        return result;
    }
    // Unary minus
    friend vec operator-(const vec& vv)
    {
        vec result;
        for(unsigned ii=0; ii<N; ++ii)
            result[ii] = -vv[ii];
        return result;
    }
    // Scalar multiplication
    friend vec operator*(T scalar, const vec& v0)
    {
        vec result;
        for(unsigned ii=0; ii<N; ++ii)
            result[ii] = scalar*v0[ii];
        return result;
    }
    friend vec operator*(const vec& v0, T scalar)
    {
        return scalar*v0;
    }
    friend vec operator/(const vec& v0, T scalar)
    {
        vec result;
        for(unsigned ii=0; ii<N; ++ii)
            result[ii] = v0[ii]/scalar;
        return result;
    }
    // NO scalar addition/subtraction, cast to correct type

    // Component-wise product
    vec operator*(const vec& right) const
    {
        vec result;
        for(unsigned ii=0; ii<N; ++ii)
            result[ii] = value_[ii]*right[ii];
        return result;
    }

    // Maths helpers
    T dot(const vec& right) const
    {
        T result = T(0);
        for(unsigned ii=0; ii<N; ++ii)
            result += value_[ii]*right[ii];
        return result;
    }
    T norm() const
    {
        //return sqrt(dot(vec(*this)));
        T result = T(0);
        for(unsigned ii=0; ii<N; ++ii)
            result += value_[ii]*value_[ii];
        return sqrt(result);
    }
    T norm2() const
    {
        //return dot(vec(*this));
        T result = T(0);
        for(unsigned ii=0; ii<N; ++ii)
            result += value_[ii]*value_[ii];
        return result;
    }
    T min() const
    {
        T result = value_[0];
        for(unsigned ii=1; ii<N; ++ii)
            if(value_[ii]<result) result=value_[ii];
        return result;
    }
    T max() const
    {
        T result = value_[0];
        for(unsigned ii=1; ii<N; ++ii)
            if(value_[ii]>result) result=value_[ii];
        return result;
    }
    T sum() const
    {
        T result = T(0);
        for(unsigned ii=1; ii<N; ++ii)
            result += value_[ii];
        return result;
    }
    inline T mean() const
    {
        return sum()/N;
    }

    // Enabled for 3D vectors
    template <bool U=true, typename=typename std::enable_if<U&&(N==3)>::type>
    inline vec cross(const vec& right) const
    {
        T x = value_[1] * right[2] - value_[2] * right[1];
        T y = value_[2] * right[0] - value_[0] * right[2];
        T z = value_[0] * right[1] - value_[1] * right[0];
        return vec(x, y, z);
    }
    template <bool U=true, typename=typename std::enable_if<U&&(N==3)>::type>
    inline vec& rotate(T radians, const vec& axis)
    {
        const T sinAlpha = sin(-radians);
        const T cosAlpha = cos(-radians);
        // Rotate around local x             y                  z
        (*this) = cross(axis*sinAlpha) + (*this * cosAlpha) + axis * dot(axis * (1 - cosAlpha));
        return (*this);
    }
    template <bool U=true, typename=typename std::enable_if<U&&(N==3)>::type>
    inline vec rotated(T radians, const vec& axis) const
    {
        return (vec(*this)).rotate(radians, axis);
    }

    // Transformations
    const vec& normalize()
    {
        T n = norm();
        for(unsigned ii=0; ii<N; ++ii)
            value_[ii] /= n;
        return *this;
    }
    vec normalized() const
    {
        return (vec(*this)).normalize();
    }
    inline vec lerp(const vec& right, T param) const
    {
        return param*(right - *this) + *this;
    }
    const vec& zero_clip()
    {
        for(unsigned ii=0; ii<N; ++ii)
            value_[ii] = detail::ZC(value_[ii]);
        return *this;
    }
    void zero()
    {
        for(unsigned ii=0; ii<N; ++ii)
            value_[ii] = T(0);
    }

    // Display
    friend std::ostream& operator<<(std::ostream& stream, const vec& right)
    {
        stream << "(";
        for(unsigned ii=0; ii<N; ++ii)
        {
            stream << right[ii];
            if(ii<N-1)
                stream << ", ";
        }
        stream << ")";
        return stream;
    }

protected:
    T value_[N];
};

using vec2 = vec<2>;
using vec3 = vec<3>;
using vec4 = vec<4>;
using i32vec2 = vec<2,uint32_t>;
using i32vec3 = vec<3,uint32_t>;
using i32vec4 = vec<4,uint32_t>;

/*
                    ___  ___      _        _
                    |  \/  |     | |      (_)
                    | .  . | __ _| |_ _ __ ___  __
                    | |\/| |/ _` | __| '__| \ \/ /
                    | |  | | (_| | |_| |  | |>  <
                    \_|  |_/\__,_|\__|_|  |_/_/\_\
*/

// OpenGL/D3D memory layout for matrices is like so:
// {[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [transX, transY, transZ, 1] }
// this is column-major, and we adopt the same memory layout.
// However we use row major notation for initialization as it is
// closer to what we would write on paper.

/**
 * @brief NxN Matrix class
 * @details Matrix class for 3D maths. Only square NxN matrices are defined.
 *
 * @tparam N Dimension of matrix
 * @tparam T=float Underlying type
 */
template <unsigned N, typename T=float>
class mat
{
private:
    /**
     * @brief Arglist ctor helper func
     * @details Pushes a list of T arguments recursively into matrix.
     *
     * @param pos Position in argument list
     * @param first First argument
     * @param args Following arguments
     */
    template <typename... Args>
    void push(unsigned pos, T first, Args&&... args)
    {
        if(pos==Size) return;   // Supernumerary arguments
        unsigned row = pos%N;
        unsigned col = pos/N;
        value_[row*N+col] = T(first);
        push(++pos, std::forward<Args>(args)...);
    }

    /**
     * @brief When no more argument in push()'s arglist, branches here.'
     *
     * @param pos Last initialized position.
     */
    void push(unsigned pos)
    {
        if(pos!=Size) // Not enough arguments, fill with zeros
        {
            for(;pos<Size;++pos)
                value_[pos] = T(0);
        }
    }

    /**
     * @brief Initialize a matrix COLUMN BY COLUMN with list of vec<N,T>.
     * @details Pushes a list of columns into a matrix.
     *
     * @param pos Position in vector argument list
     * @param first First column vector
     * @param args Following vectors
     */
    template <typename... Args>
    void pushvec(unsigned pos, vec<N,T> first, Args&&... args)
    {
        if(pos==N) return;   // Supernumerary arguments
        for(unsigned ii = 0; ii < N; ++ii) {
            value_[pos*N+ii] = first[ii];
        }
        pushvec(++pos, std::forward<Args>(args)...);
    }

    /**
     * @brief When no more argument in pushvec()'s arglist, branches here.'
     *
     * @param pos Last initialized position.
     */
    void pushvec(unsigned pos)
    {
        if(pos!=N) // Not enough arguments, fill with zeros
        {
            for(;pos<N;++pos)
                for(unsigned ii=0; ii<N; ++ii)
                    value_[pos*N+ii] = T(0);
        }
    }

public:
    static const unsigned Size = N*N;

    // Constructors
    /**
     * @brief Default ctor.
     * @details Initialize matrix with all zeros.
     */
    mat()
    {
        for(unsigned ii=0; ii<Size; ++ii)
            value_[ii] = T(0);
    }

    /**
     * @brief Unique value constructor
     * @details Initialize matrix with a given value everywhere.
     *
     * @param unique_value The value.
     */
    explicit mat(T unique_value)
    {
        for(unsigned ii=0; ii<Size; ++ii)
            value_[ii] = unique_value;
    }

    /**
     * @brief Arglist ctor
     * @details Construct a matrix from a list of arguments. If not enough
     * arguments given, the remaining coefficients are set to 0.
     *
     * @param first First parameter
     * @param args The remaining parameters
     */
    template <typename... Args>
    explicit mat(T first, Args&&... args)
    {
        push(0, first, args...);
    }

    /**
     * @brief Column constructor.
     * @details Construct a matrix from a list of COLUMN vectors.
     *
     * @param first_col First column vector.
     * @param args The other vectors.
     */
    template <typename... Args>
    explicit mat(vec<N,T> first_col, Args&&... args)
    {
        pushvec(0, first_col, args...);
    }

    /**
     * @brief Move constructor.
     * @details Construct a matrix from an r-value matrix
     *
     * @param right Other matrix.
     */
    /*mat(mat&& right)
    {
        std::swap(value_, right.value_);
    }*/

    /**
     * @brief Copy constructor.
     * @details Copy a matrix
     *
     * @param right Other matrix.
     */
    /*mat(const mat& right)
    {
        std::copy(right.value_,right.value_+Size,value_);
    }*/

    /**
     * @brief Parentheses accessor.
     * @details Get a reference to an element by specifying the row and column
     * indices.
     *
     * @param row Row index.
     * @param col Column index.
     *
     * @return Reference to value at (row, col)
     */
    inline T& operator()(unsigned row, unsigned col)
    {
        assert(row<N && "mat::operator(): row index out of bound");
        assert(col<N && "mat::operator(): col index out of bound");
        return value_[col*N+row];
    }

    /**
     * @brief Parentheses accessor.
     * @details Access a matrix element by specifying the row and column
     * indices.
     *
     * @param row Row index.
     * @param col Column index.
     *
     * @return Value at (row, col)
     */
    inline T operator()(unsigned row, unsigned col) const
    {
        assert(row<N && "mat::operator() const: row index out of bound");
        assert(col<N && "mat::operator() const: col index out of bound");
        return value_[col*N+row];
    }

    /**
     * @brief Return reference to matrix value at index.
     *
     * @param index Column major index.
     */
    inline T& operator[](unsigned index)
    {
        assert(index<Size && "mat::operator[]: index out of bound");
        return value_[index];
    }

    /**
     * @brief Return matrix value at index.
     *
     * @param index Column major index.
     */
    inline T operator[](unsigned index) const
    {
        assert(index<Size && "mat::operator[]: index out of bound");
        return value_[index];
    }

    /**
     * @brief Get a pointer to the first element in memory.
     *
     * @return const pointer to first element.
     */
    inline T const* get_pointer() const
    {
        return value_;
    }

    /**
     * @brief Copy right matrix to self.
     *
     * @param right matrix on the right of equal sign.
     */
    /*inline mat& operator=(const mat& right)
    {
        std::copy(right.value_, right.value_+Size, value_);
        return *this;
    }*/

    /**
     * @brief Move right matrix to self.
     *
     * @param right matrix on the right of equal sign.
     */
    /*inline mat& operator=(mat&& right)
    {
        std::swap(value_, right.value_);
        return *this;
    }*/

    /**
     * @brief Fill matrix with a single value.
     *
     * @param unique_value Unique value to fill the matrix with.
     */
    inline mat& operator=(T unique_value)
    {
        for(unsigned ii = 0; ii < Size; ++ii) {
            value_[ii] = unique_value;
        }
        return *this;
    }

    /**
     * @brief Check whether matrix corresponds to an affine transformation.
     *
     * @return true if matrix is affine, else false
     */
    bool is_affine() const
    {
        if(value_[Size-1]!=T(1)) return false;
        for(unsigned jj=0; jj<N-1; ++jj)
            if(value_[jj*N + N-1]) return false;
        return true;
    }


    /**
     * @brief Return submatrix given pivots.
     * @details Return what's left of the matrix if we remove the row
     * and column at pivot.
     *
     * @param center_row Pivot row.
     * @param center_col Pivot column.
     *
     * @return (N-1)x(N-1) submatrix.
     */
    template <bool U=true, typename=typename std::enable_if<U&&(N>2)>::type>
    inline mat<N-1,T> submatrix(unsigned center_row, unsigned center_col) const
    {
        mat<N-1,T> result;
        unsigned ii_offset = 0;
        for(unsigned ii=0; ii<N-1; ++ii)
        {
            unsigned jj_offset = 0;
            if(ii>=center_row) ii_offset=1;
            for(unsigned jj=0; jj<N-1; ++jj)
            {
                if(jj>=center_col) jj_offset=1;
                result(ii,jj) = this->operator()(ii+ii_offset,jj+jj_offset);
            }
        }
        return result;
    }

    /**
     * @brief Return indexed column of matrix as a N-vector.
     *
     * @param index Column number.
     * @return Vector of dimension N copied from matrix.
     */
    inline vec<N,T> col(unsigned index) const
    {
        assert(index<N && "mat::col(): Index out of bounds.");
        vec<N,T> result;
        for(unsigned jj=0; jj<N; ++jj)
            result[jj] = value_[index*N+jj];
        return result;
    }

    /**
     * @brief Return indexed row of matrix as a N-vector.
     *
     * @param index Row number.
     * @return Vector of dimension N copied from matrix.
     */
    inline vec<N,T> row(unsigned index) const
    {
        assert(index<N && "mat::row(): Index out of bounds.");
        vec<N,T> result;
        for(unsigned ii=0; ii<N; ++ii)
            result[ii] = value_[ii*N+index];
        return result;
    }

    /**
     * @brief Initialize the diagonal elements with input vector values.
     * @details All non-diagonal values are set to 0.
     *
     * @param diag N-vector specifying the matrix diagonal.
     * @return The matrix itself.
     */
    const mat& init_diagonal(const vec<N,T>& diag)
    {
        this->operator=(0.0f);
        for(unsigned ii=0; ii<N; ++ii)
            value_[(N+1)*ii] = diag[ii];
        return *this;
    }

    /**
     * @brief Initialize matrix diagonal with all ones.
     * @details All non-diagonal values are set to 0.
     *
     * @return The matrix itself.
     */
    const mat& init_identity()
    {
        this->operator=(0.0f);
        for(unsigned ii=0; ii<N; ++ii)
            value_[(N+1)*ii] = T(1);
        return *this;
    }

    /**
     * @brief Init as a scale matrix
     * @details All diagonal elements except the last one (which is set
     * to 1) are initialized according to the scales (N-1)-vector.
     *
     * @param scales Input (N-1)-vector.
     * @return The matrix itself.
     */
    const mat& init_scale(const vec<N-1,T>& scales)
    {
        this->operator=(0.0f);
        for(unsigned ii=0; ii<N-1; ++ii)
            value_[(N+1)*ii] = scales[ii];
        value_[Size-1] = T(1);
        return *this;
    }

    /**
     * @brief Init as a scale matrix
     * @details All diagonal elements except the last one (which is set
     * to 1) are homogeneously set to scale.
     *
     * @param scale Input scale.
     * @return The matrix itself.
     */
    const mat& init_scale(T scale)
    {
        this->operator=(0.0f);
        for(unsigned ii=0; ii<N-1; ++ii)
            value_[(N+1)*ii] = scale;
        value_[Size-1] = T(1);
        return *this;
    }

    /**
     * @brief Make this matrix a translation matrix in homogenous coordinates.
     * @details This is an identity matrix with the right-most vector replaced
     * by the translation vector [x y z 1.0]
     *
     * @return The matrix itself.
     */
    const mat& init_translation(const vec<N-1,T>& translation)
    {
        init_identity();
        for(unsigned ii=0; ii<N-1; ++ii)
            value_[(N-1)*N+ii] = translation[ii];
        return *this;
    }

    /**
     * @brief Transpose matrix.
     * @details All elements are flipped with respect to the diagonal.
     *
     * @return The matrix itself.
     */
    const mat& transpose()
    {
        mat Matrix(*this);
        for(unsigned jj=0; jj<N; ++jj)
        {
            for(unsigned ii=0; ii<N; ++ii)
            {
                if(ii==jj) continue;
                value_[jj*N + ii] = Matrix(jj,ii);
            }
        }
        return *this;
    }

    /**
     * @brief Returns the transposed matrix.
     * @details All elements are flipped with respect to the diagonal.
     * Creates a new matrix.
     *
     * @return Transposed matrix.
     */
    mat transposed() const
    {
        return (mat(*this)).transpose();
    }

    /**
     * @brief Set elements to exactly 0 when they are close enough to 0.
     *
     * @return The matrix itself.
     */
    const mat& zero_clip()
    {
        for(unsigned ii=0; ii<Size; ++ii)
            value_[ii] = detail::ZC(value_[ii]);
        return *this;
    }

    /**
     * @brief Square NxN Matrix multiplication.
     * @details See linear algebra 101!
     *
     * @param right The right side matrix
     * @return A new matrix which is the product (*this) * right
     */
    mat<N,T> operator*(const mat<N,T>& right) const
    {
        mat<N,T> result(T(0));
        for (unsigned jj = 0; jj<N; ++jj)
            for (unsigned ii = 0; ii<N; ++ii)
                for (unsigned kk = 0; kk<N; ++kk)
                    result[jj*N+ii] += value_[kk*N+ii] * right[jj*N+kk];
        return result;
    }

    friend mat<N,T> operator*(const mat<N,T>& lhs, float rhs)
    {
        mat<N,T> ret(lhs);
        for (unsigned jj = 0; jj<ret.Size; ++jj)
            ret[jj] *= rhs;
        return ret;
    }
    friend mat<N,T> operator*(float lhs, const mat<N,T>& rhs)
    {
        mat<N,T> ret(rhs);
        for (unsigned jj = 0; jj<ret.Size; ++jj)
            ret[jj] *= lhs;
        return ret;
    }

    mat<N,T> operator+(const mat<N,T>& right) const
    {
        mat<N,T> result(*this);
        for (unsigned jj = 0; jj<Size; ++jj)
            result[jj] = value_[jj] + right.value_[jj];
        return result;
    }

    mat<N,T> operator-(const mat<N,T>& right) const
    {
        mat<N,T> result(*this);
        for (unsigned jj = 0; jj<Size; ++jj)
            result[jj] = value_[jj] - right.value_[jj];
        return result;
    }

    /**
     * @brief Set this matrix to the product of itself times the right side matrix.
     *
     * @param right The right side matrix.
     * @return The matrix itself (this).
     */
    const mat& operator*=(const mat& right)
    {
        (*this) = (*this)*right;
        return *this;
    }

    /**
     * @brief Transform input N-vector by PRE-multiplying it with this matrix.
     *
     * @param right Input N-vector.
     * @return Output N-vector.
     */
    vec<N,T> operator*(const vec<N,T>& right) const
    {
        vec<N,T> result;
        for(unsigned jj=0; jj<N; ++jj)
            for(unsigned ii=0; ii<N; ++ii)
                result[ii]+=value_[jj*N+ii]*right[jj];
        return result;
    }

    /**
     * @brief Same as vector transform but allows vector to be of dimension N-1.
     * @details Allows to multiply (for example) a 3D space vector by
     * a 4D homogenous matrix. Input vector is augmented with homogenous
     * coordinate w=1, matrix-vector multiplication is done and a 3D vector
     * is extracted. USE ONLY WITH AFFINE MATRICES.
     *
     * @param right Input (N-1)-vector
     * @return Output (N-1)-vector
     */
    vec<N-1,T> operator*(const vec<N-1,T>& right) const
    {
        vec<N,T> augmented(right, 1.0);
        return vec<N-1,T>(this->operator*(augmented));
    }

    /**
     * @brief Display matrix to console with fancy ASCII shit.
     *
     * @param stream A ostream object like std::cout
     * @param right A NxN matrix
     * @return The input stream object so we can chain this operator.
     */
    friend std::ostream& operator<<(std::ostream& stream, const mat& right)
    {
        // Compute maximum digit number for a column
        std::stringstream ss;
        size_t max_digit[N];
        for(unsigned jj=0; jj<N; ++jj) max_digit[jj] = 0;
        for(unsigned jj=0; jj<N; ++jj) // column
        {
            for(unsigned ii=0; ii<N; ++ii) // row
            {
                // Here we simulate the string of digits being pushed to the stream
                ss << right(ii,jj);
                size_t ndigit = ss.str().size();
                // and store the biggest string size for that column
                if(ndigit>max_digit[jj]) max_digit[jj] = ndigit;

                ss.str(std::string());
                ss.clear();
            }
        }

        // Display
        for(unsigned ii=0; ii<N; ++ii)
        {
            if     (ii==0)      stream << "/ ";
            else if(ii==N-1) stream << "\\ ";
            else                stream << "| ";
            for(unsigned jj=0; jj<N; ++jj)
            {
                // Column width is set to the maximum digits number in said column
                stream << std::left << std::setw(max_digit[jj]);
                stream << right(ii,jj);
                if(jj<N-1) stream << "  ";
            }
            if     (ii==0)      stream << " \\";
            else if(ii==N-1) stream << " /";
            else                stream << " |";
            if(ii<N-1) stream << std::endl;
        }

        return stream;
    }

    /**
     * @brief Display matrix inline as represented in memory.
     *
     * @param stream Where to stream to (like std::cout).
     */
    void display_memlayout(std::ostream& stream)
    {
        stream << "[";
        for(unsigned ii = 0; ii < Size; ++ii) {
            stream << value_[ii];
            if (ii==Size-1) break;
            else if(ii%N == N-1) stream << " ; ";
            else stream << " ";
        }
        stream << "]" << std::endl;
    }


private:
    T value_[Size];
};


using mat2 = mat<2>;
using mat3 = mat<3>;
using mat4 = mat<4>;

struct Frustum
{
public:
    Frustum(float left, float right, float bottom, float top, float near, float far):
    l(left), r(right), b(bottom), t(top), n(near), f(far),
    w(right-left), h(top-bottom), d(far-near){}

    void init(float left, float right, float bottom, float top, float near, float far)
    {
        l = left;
        r = right;
        b = bottom;
        t = top;
        n = near;
        f = far;
        w = r-l;
        h = t-b;
        d = f-n;
    }

    float l; // left
    float r; // right
    float b; // bottom
    float t; // top
    float n; // near
    float f; // far

    float w; // width
    float h; // height
    float d; // depth
};

}

namespace detail
{
    inline uint32_t Hash(float f)
    {
        union // Used to reinterpret float mantissa as an unsigned integer...
        {
            float    f_;
            uint32_t u_;
        };
        f_ = f;

        // ... with a 3 LSB epsilon (floats whose mantissas are only 3 bits different will share a common hash)
        return u_ & 0xfffffff8; // BEWARE: Depends on endianness
    }
    inline uint32_t Hash(uint32_t x)
    {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x;
    }
    inline uint32_t Unhash(uint32_t x)
    {
        x = ((x >> 16) ^ x) * 0x119de1f3;
        x = ((x >> 16) ^ x) * 0x119de1f3;
        x = (x >> 16) ^ x;
        return x;
    }
    inline uint64_t Hash(uint64_t x)
    {
        x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
        x = x ^ (x >> 31);
        return x;
    }
    inline uint64_t Unhash(uint64_t x)
    {
        x = (x ^ (x >> 31) ^ (x >> 62)) * UINT64_C(0x319642b2d24d8ec3);
        x = (x ^ (x >> 27) ^ (x >> 54)) * UINT64_C(0x96de1b173f119089);
        x = x ^ (x >> 30) ^ (x >> 60);
        return x;
    }

} // namespace detail
} // namespace wcore

namespace std
{
    // Allows to use std::swap() on vecs
    template <unsigned N, typename T>
    inline void swap(::wcore::math::vec<N,T>& left, ::wcore::math::vec<N,T>& right)
    { left.swap(right); }

    // math::vecN Hasher
    // enables the use of unordered containers with vecs
    template <unsigned N, typename T>
    struct hash<::wcore::math::vec<N,T>>
    {
        typedef ::wcore::math::vec<N,T> argument_type;
        typedef uint32_t result_type;

        result_type operator()(argument_type const& arg) const
        {
            unsigned int seed = 0;

            // Combine component hashes to obtain a position hash
            // Similar to Boost's hash_combine function
            for(unsigned ii = 0; ii < N; ++ii) {
                seed ^= ::wcore::detail::Hash(arg[ii]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }

            return seed; // Two epsilon-distant vertices will share a common hash
        }
    };
}


#endif // MATH_STRUCTURES_H
