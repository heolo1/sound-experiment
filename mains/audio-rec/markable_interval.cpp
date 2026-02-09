#include "markable_interval.hpp"

#include <cstdint>
#include <tuple>
#include <vector>

template <typename T>
markable_interval<T>::markable_interval(const T &start, const T &end, bool start_inclusive, bool end_inclusive)
    : start_{start}, end_{end}, start_inclusive_{start_inclusive}, end_inclusive_{end_inclusive} {}

/**
 * We want an indexing function such that when we call indexing(val, inclusivity), 
 * it returns an index corresponding to the next demarcation boundary.
 * 
 * We assume that the boundaries are always valid.
 * Maybe instead of inclusivity, we need to pass in a left vs. right argument. (Post: we need both)
 */
template <typename T>
std::size_t markable_interval<T>::index(const T &val, mark_type mark_type) const {
    // do a binary search through marks_
    std::size_t start = 0, end = marks_.size();
    std::size_t target;
    while ((target = (end + start) / 2) != start) {
        if (get<0>(marks_[target]) == val) {
            break;
        } else if (get<0>(marks_[target]) < val) {
            start = target + 1;
        } else {
            end = target;
        }
    }

    /**
     * Case 1: target = marks_.size() -> get<0>(marks_) < val, return target
     * Case 2: get<0>(marks_[target]) == val -> complicated
     * Case 3: get<0>(marks_[target]) < val -> return target + 1 (get<0>(marks_[target + 1]) will be end or > val)
     * Case 4: get<0>(marks_[target]) > val -> return target
     * 
     * marks_.size() == 0 is covered here by virtue of target being <= marks_.size()
     */
    if (target == marks_.size() || get<0>(marks_[target]) > val) {
        return target;
    } else if (get<0>(marks_[target]) < val) {
        return target + 1;
    }

    /**
     * Assume that the following scenarios exist in demarcation buffer:
     * a. BG, 0), (0, ED
     * b. BG, 0), ED
     * c. BG, (0, ED
     * d. BG, [0, 0], ED
     * e. BG, [0, ED
     * f. BG, 0], ED
     * 
     * We should never encounter (0, 0] or [0, 0)
     * 
     * Indexing of (0 should return a. ED b. ED c. ED d. ED e. ED f. ED
     * Indexing of 0) should return a. 0) b. 0) c. (0 d. [0 e. [0 f. 0]
     * Indexing of [0 should return a. (0 b. ED c. (0 d. 0] e. ED f. 0]
     * Indexing of 0] should return a. (0 b. ED c. (0 d. 0] e. ED f. 0]
     * Indexing of 0^ should return a. (0 b. ED c. (0 d. 0] e. ED f. 0]
     * 
     * Where 0^ indicates exactness
     * Note how [0, 0], and 0^ are identical
     */

    // we probably first want to move target to the last marching marks_[target] in the case of (a)
    if (target < marks_.size() - 1 && get<0>(marks_[target]) == get<0>(marks_[target + 1])) {
        target++;
    }

    /**
     * tackle order:
     *    abcdef
     * (0 ------
     * 0) *--*--
     * 0* -*--*-
     * 
     * - * indicates a special case
     * - 0* indicates [0, 0], and 0^
     */
    
    if (mark_type == start_exclusive) { // all of (0
        return target + 1;
    }

    bool is_double_bound = target != 0 && get<0>(marks_[target - 1]) == get<0>(marks_[target]);
    if (mark_type == end_exclusive) { // all of 0), * handled here as well
        // target != 0 && get<0>(marks_[target - 1]) == get<0>(marks_[target]) is condition for (a, d)
        return target - is_double_bound;
    }

    // all of 0*, xor covers *
    return target + (get<1>(marks_[target]) ^ index_is_marked(target));
}

template <typename T>
void markable_interval<T>::mark(const T &start, const T &end, bool start_inclusive, bool end_inclusive) {
    /**
     * Case 1: both start and end are currently unmarked
     * Case 2: either start or end are currently unmarked
     * Case 3: both start and end are currently marked
     */

    auto start_index = index(start, mark_type_start(start_inclusive));
    auto end_index = index(end, mark_type_end(end_inclusive));

    marks_.erase(marks_.begin() + start_index, marks_.begin() + end_index);

    // end_index has moved to start_index
    if (!index_is_marked(end_index)) {
        if (marks_.size() > start_index && get<0>(marks_[start_index]) == end && (end_inclusive || get<1>(marks_[start_index]))) {
            // remove the next bound if we reach a situation along the lines of 0], (0
            marks_.erase(marks_.begin() + start_index);
        } else {
            marks_.emplace(marks_.begin() + start_index, end, end_inclusive);
        }
    }
    
    if (start_index == 0) {
        return;
    }

    if (!index_is_marked(start_index)) {
        start_index--; // we now look one before start_index for a 0], start_index: (0
        if (get<0>(marks_[start_index]) == start && (start_inclusive || get<1>(marks_[start_index]))) {
            marks_.erase(marks_.begin() + start_index);
        } else {
            marks_.emplace(marks_.begin() + start_index, start, start_inclusive);
        }
    }
}

template <typename T>
void markable_interval<T>::unmark(const T &start, const T &end, bool start_inclusive, bool end_inclusive) {
    auto start_index = index(start, mark_type_start(start_inclusive));
    auto end_index = index(end, mark_type_end(end_inclusive));

    marks_.erase(marks_.begin() + start_index, marks_.begin() + end_index);

    // look for things happening around the edges like in markable_interval::mark
    if (index_is_marked(end_index)) {
        if (marks_.size() > start_index && get<0>(marks_[start_index]) == end && (end_inclusive || get<1>(marks_[start_index]))) {
            // remove the next bound if we reach a situation along the lines of 0], (0
            marks_.erase(marks_.begin() + start_index);
        } else {
            marks_.emplace(marks_.begin() + start_index, end, end_inclusive);
        }
    }
    
    if (start_index == 0) {
        return;
    }
    
    if (index_is_marked(start_index)) {
        start_index--; // we now look one before start_index for a 0], start_index: (0
        if (get<0>(marks_[start_index]) == start && (start_inclusive || get<1>(marks_[start_index]))) {
            marks_.erase(marks_.begin() + start_index);
        } else {
            marks_.emplace(marks_.begin() + start_index, start, start_inclusive);
        }
    }
}


template <typename T>
void markable_interval<T>::mark(const T &val) {
    mark(val, val, true, true);
}

template <typename T>
void markable_interval<T>::unmark(const T &val) {
    unmark(val, val, true, true);
}

template <typename T>
bool markable_interval<T>::is_marked(const T &val) const {
    return index_is_marked(index(val, exact)); // check if the next mark is an end
}

template <typename T>
bool markable_interval<T>::is_marked(const T &start, const T &end, bool start_inclusive, bool end_inclusive) const {
    auto start_index = index(start, mark_type_start(start_inclusive));
    return (index_is_marked(start_index) && start_index == index(end, mark_type_end(end_inclusive)));
}

template <typename T>
bool markable_interval<T>::is_partially_marked(const T &start, const T &end, bool start_inclusive, bool end_inclusive) const {
    auto start_index = index(start, mark_type_start(start_inclusive));
    return (index_is_marked(start_index) || start_index != index(end, mark_type_end(end_inclusive)));
}

template <typename T>
bool markable_interval<T>::is_in_bounds(const T &val) const {
    return (start_ < val || (start_ == val && start_inclusive_)) && (val < end_ || (val == end_ && end_inclusive_)); 
}

template <typename T>
bool markable_interval<T>::is_in_bounds(const T &start, const T &end, bool start_inclusive, bool end_inclusive) const {
    return (end < end_ || (end == end_ && (end_inclusive_ || !end_inclusive)))
        && (start_ < start || (start_ == start && (start_inclusive_ || !start_inclusive))); 
}

template <typename T>
bool markable_interval<T>::is_partially_in_bounds(const T &start, const T &end, bool start_inclusive, bool end_inclusive) const {
    return is_in_bounds(end, start, end_inclusive, start_inclusive);
}

template <typename T>
bool markable_interval<T>::set_start(const T &start) {
    if (end_ < start) {
        start_ = end_;
    } else {
        start_ = start;
    }
    return !(end_ < start);
}

template <typename T>
bool markable_interval<T>::set_end(const T &end) {
    if (end < start_) {
        end_ = start_;
    } else {
        end_ = end;
    }
    return !(end_ < start_);
}

template <typename T>
void markable_interval<T>::set_is_start_inclusive(bool start_inclusive) {
    start_inclusive_ = start_inclusive;
}

template <typename T>
void markable_interval<T>::set_is_end_inclusive(bool end_inclusive) {
    end_inclusive_ = end_inclusive;
}

template <typename T>
const T &markable_interval<T>::get_start() const {
    return start_;
}

template <typename T>
const T &markable_interval<T>::get_end() const {
    return end_;
}

template <typename T>
bool markable_interval<T>::is_start_inclusive() const {
    return start_inclusive_;
}

template <typename T>
bool markable_interval<T>::is_end_inclusive() const {
    return end_inclusive_;
}

template struct markable_interval<uint32_t>;