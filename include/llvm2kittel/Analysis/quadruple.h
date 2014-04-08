// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef QUADRUPLE_H
#define QUADRUPLE_H

/// This class represents a quadruple, in analogy to std::pair.
template<class T1, class T2, class T3, class T4>
struct quadruple
{
    /// First component
    T1 first;
    /// Second component
    T2 second;
    /// Third component
    T3 third;
    /// Fourth component
    T4 fourth;

    /// Constructor
    quadruple(const T1 &x, const T2 &y, const T3 &z, const T4 &a)
      : first(x), second(y), third(z), fourth(a)
    {}

    /// Copy constructor
    template <class U1, class U2, class U3, class U4>
    quadruple(const quadruple<U1, U2, U3, U4> &q)
      : first(q.first), second(q.second), third(q.third), fourth(q.fourth)
    {}
};

/// Convenience function for construction of a quadruple.
/// \relates quadruple
template<class T1, class T2, class T3, class T4>
quadruple<T1, T2, T3, T4> make_quadruple(T1 x, T2 y, T3 z, T4 a)
{
    return quadruple<T1, T2, T3, T4>(x, y, z, a);
}

/// Two quadruples of the same type are equal iff their members are equal.
/// \relates quadruple
template<class T1, class T2, class T3, class T4>
bool operator==(const quadruple<T1, T2, T3, T4> &x, const quadruple<T1, T2, T3, T4> &y)
{
    return x.first == y.first && x.second == y.second && x.third == y.third && x.fourth == y.fourth;
}

/// Lexicographic comparison.
/// \relates quadruple
template<class T1, class T2, class T3, class T4>
bool operator<(const quadruple<T1, T2, T3, T4> &x, const quadruple<T1, T2, T3, T4> &y)
{
    return x.first < y.first
        || (!(y.first < x.first) && x.second < y.second)
        || (!(y.first < x.first) && !(y.second < x.second) && x.third < y.third)
        || (!(y.first < x.first) && !(y.second < x.second) && !(y.third < x.third) && x.fourth < y.fourth);
}

/// Negation of operator==().
/// \relates quadruple
template<class T1, class T2, class T3, class T4>
bool operator!=(const quadruple<T1, T2, T3, T4> &x, const quadruple<T1, T2, T3, T4> &y)
{
    return !(x == y);
}

/// Uses operator<() to find the result.
/// \relates quadruple
template<class T1, class T2, class T3, class T4>
bool operator>(const quadruple<T1, T2, T3, T4> &x, const quadruple<T1, T2, T3, T4> &y)
{
    return y < x;
}

/// Uses operator<() to find the result.
/// \relates quadruple
template<class T1, class T2, class T3, class T4>
bool operator<=(const quadruple<T1, T2, T3, T4> &x, const quadruple<T1, T2, T3, T4> &y)
{
    return !(y < x);
}

/// Uses operator<() to find the result.
/// \relates quadruple
template<class T1, class T2, class T3, class T4>
bool operator>=(const quadruple<T1, T2, T3, T4> &x, const quadruple<T1, T2, T3, T4> &y)
{
    return !(x < y);
}

#endif
