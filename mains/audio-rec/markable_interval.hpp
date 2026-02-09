#pragma once

#include <concepts>
#include <cstdint>
#include <tuple>
#include <vector>

/**
 * a class for marking intervals of numbers on a given range
 * 
 * internally, we operate on the assumption that if the start or end are marked, then there exists a demarcator in marked_ for it
 * 
 */
template <typename T>
struct markable_interval {
    enum mark_type {
        exact,
        start_exclusive,
        start_inclusive,
        end_exclusive,
        end_inclusive,
    };

    inline static mark_type mark_type_start(bool inclusive) {
        return inclusive ? start_inclusive : start_exclusive;
    }

    inline static mark_type mark_type_end(bool inclusive) {
        return inclusive ? end_inclusive : end_exclusive;
    }

    struct discrete_it;
    struct discrete_it_iter;

    markable_interval(const T &start, const T &end, bool start_inclusive = true, bool end_inclusive = false);

    void mark     (const T &start, const T &end, bool start_inclusive = true, bool end_inclusive = false);
    void unmark   (const T &start, const T &end, bool start_inclusive = true, bool end_inclusive = false);
    void mark     (const T &);
    void unmark   (const T &);
    
    bool is_marked(const T &) const;
    bool is_marked(const T &start, const T &end, bool start_inclusive = true, bool end_inclusive = false) const;
    bool is_partially_marked(const T &start, const T &end, bool start_inclusive = true, bool end_inclusive = false) const;
    
    bool is_in_bounds(const T &) const;
    bool is_in_bounds(const T &start, const T &end, bool start_inclusive = true, bool end_inclusive = false) const; // is entirely in bounds, validity of [start, end] is assumed
    bool is_partially_in_bounds(const T &start, const T &end, bool start_inclusive = true, bool end_inclusive = false) const; // some portion is in bounds

    bool set_start(const T &); // returns true if this modification is valid. if not, then start is set to end
    bool set_end  (const T &);   // returns true if this modification is valid. if not, then end is set to start
    void set_is_start_inclusive(bool);
    void set_is_end_inclusive  (bool);
    
    const T &get_start() const;
    const T &get_end  () const;
    bool is_start_inclusive() const;
    bool is_end_inclusive  () const;

    discrete_it iter() const requires std::integral<T>;

private:
    std::size_t index(const T &, mark_type) const;
    inline bool index_is_marked(std::size_t) const;

    T start_, end_;
    std::vector<std::tuple<T, bool>> marks_; // bound, inclusivity
    bool start_inclusive_, end_inclusive_;

    friend discrete_iter;
};

template <typename T>
inline bool markable_interval<T>::index_is_marked(std::size_t s) const {
    return s % 2;
}

template <typename T>
struct markable_interval<T>::discrete_it {
    discrete_it(const markable_interval<T> &);

    discrete_it_iter begin() const;
    discrete_it_iter end() const;

private:
    const markable_interval<T> &marks_;
};

template <typename T>
struct markable_interval<T>::discrete_it_iter {
    discrete_it_iter(const markable_interval<T> &, T);

    auto &operator<=>(const discrete_it_iter &) const;
    const T &operator*() const;
    discrete_it_iter &operator++();

private:
    const std::vector<std::tuple<T, bool>> &marks_;
    T val;
};

extern template struct markable_interval<uint32_t>;