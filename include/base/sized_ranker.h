//
// Created by zyy on 2020/12/23.
//

#ifndef NEMU_SIZED_RANKER_H
#define NEMU_SIZED_RANKER_H


#include <queue>

template <class T, class Cmp>
class SizedRanker
{
  private:
    std::priority_queue<T, std::vector<T>, Cmp> pq;
    size_t maxSize;
  public:
    void set_size (unsigned size) {maxSize = size;}
    SizedRanker()
    {
        pq=std::priority_queue<T, std::vector<T>, Cmp>();
    }
    void possiblyAdd(const T &newValue)
    {
        if (pq.size() < maxSize)
        {
            pq.push(newValue);
            return;
        }
        if(Cmp()(newValue, pq.top()))
        {
            pq.pop(); //get rid of the root
            pq.push(newValue); //priority queue will automatically restructure
        }
    }
    std::priority_queue<T, std::vector<T>, Cmp> &get()
    {
        return pq;
    }
};

#endif //NEMU_SIZED_RANKER_H
