#ifndef util_h
#define util_h

#include <HAL.h>
#include <stdlib.h>

#define GH \
  hal_printf("%s:%d %s gh\r\n", __FILE__, __LINE__, __PRETTY_FUNCTION__)

namespace util {

// Not thread safe
// No dynamic allocation
template <int size>
class rolling_average {
private:
  size_t index_ = 0;
  size_t num_data_ = 0;
  float buf_[size];

public:
  void push(float data) {
    buf_[index_] = data;

    if (num_data_ < size) num_data_++;
    index_ = (index_ + 1) % size;
  }

  float average() {
    if (num_data_ == 0) return 0.0;

    float total = 0;
    for (int i = 0; i < num_data_; i++) {
      total += buf_[i];
    }
    return total / num_data_;
  }
};

}  // namespace util

#endif
