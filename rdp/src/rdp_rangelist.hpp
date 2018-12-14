#ifndef __RDP_RANGE_LIST_HPP__
#define __RDP_RANGE_LIST_HPP__
#include <deque>
#include <algorithm>
/**
 * 范围数组
 */
template<typename T>
class RangeList : public std::deque<std::pair<T,T> >
{
public:
    typedef typename std::deque<std::pair<T,T> > super_type;
    typedef typename super_type::value_type value_type;
    typedef typename super_type::iterator iterator;
    void insert(const T& t)
    {
        insert(value_type(t,t));
    }
    void insert(const value_type& range)
    {//当前range用<>表示,数组中项用[]表示
        if (range.first > range.second)
            return;
        if (super_type::size() == 0) {
            super_type::push_back(range);//0. <>
            return;
        }
        iterator pos = std::upper_bound(super_type::begin(), super_type::end(), range);
        if (pos == super_type::end()) {
            --pos;
            if (range.first <= pos->second + 1) {
                if (pos->second < range.second)
                    pos->second = range.second;//1. [ <] >
                else
                    ;//2. [ <>]
            }
            else 
                super_type::push_back(range); //3. [] <>
        }
        else if (pos == super_type::begin()) {
            if (range.second + 1 >= pos->first) {
                iterator iter = pos;
                do {
                    if (range.second <= iter->second)
                        break;
                    ++iter;
                } while (iter != super_type::end());
                if (iter == super_type::end()){
                    super_type::erase(pos, iter);
                    super_type::push_back(range);//4. < [] [] [] ... >
                }
                else {
                    iter->first = range.first;
                    super_type::erase(pos, iter);//5. < [] [] [ >]
                }
            }
            else 
                super_type::push_front(range);//6. <> []
        }
        else {
            const iterator& prev = pos - 1;
            if (range.second + 1 >= pos->first) {
                iterator iter = pos;
                do {
                    if (range.second <= iter->second)
                        break;
                    ++iter;
                } while (iter != super_type::end());
                if (iter == super_type::end()) {
                    if (range.first <= prev->second + 1) {
                        prev->second = range.second;
                        super_type::erase(pos, iter);//7. [ <] [][] ...>
                    }
                    else {
                        super_type::erase(pos, iter);
                        super_type::push_back(range);//8. [] < [] ... >
                    }
                }
                else {
                    if (range.first <= prev->second + 1) {
                        iter->first = prev->first;
                        super_type::erase(prev, iter);//9. [ <] [] ... [ >]
                    }
                    else {
                        iter->first = range.first;
                        super_type::erase(pos, iter);//10. [] < [] ... [ >]
                    }
                }
            }
            else if (range.first <= prev->second + 1) {
                if (range.second > prev->second)
                    prev->second = range.second;//11.[ <] > []
                else
                    ;//12. [ <> ] []
            }
            else {
                super_type::insert(pos, range);//13.[] <> []
            }
        }
    }
};
#endif
