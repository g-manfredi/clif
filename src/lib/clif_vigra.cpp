#include "clif_vigra.hpp"

#include <cstddef>

#include <sstream>
#include <string>

#include "clif_cv.hpp"

namespace clif {
  
using namespace vigra;
  
Shape2 imgShape(Datastore *store)
{
  cv::Size size = imgSize(store);
  
  return Shape2(size.width, size.height);
}

template <typename T> class passthrough_allocator {
  void *data;

public:
    using value_type = T;
    using size_type = std::size_t;
    using propagate_on_container_move_assignment = std::true_type;

    //passthrough_allocator() noexcept = default;
    passthrough_allocator(void *ptr) noexcept : data(ptr) {};
    //passthrough_allocator(const passthrough_allocator&) noexcept {}
    //template <typename U>
    //passthrough_allocator(const passthrough_allocator<U>&) noexcept {}
    //passthrough_allocator(passthrough_allocator&& other) noexcept : data(other.data) {}

    passthrough_allocator& operator = (const passthrough_allocator&) noexcept
    {
        // noop
        return *this;
    }

    passthrough_allocator& operator = (passthrough_allocator&& other) noexcept
    {
        data = other.data;
        return *this;
    }

    ~passthrough_allocator() noexcept {}

    T* allocate(size_type n)
    {
      return static_cast<T*>(data);
    }

    void deallocate(T* ptr, size_type n) noexcept {}
};

template <typename T, typename U>
inline bool operator == (const passthrough_allocator<T>&, const passthrough_allocator<U>&) {
    return true;
}

template <typename T, typename U>
inline bool operator != (const passthrough_allocator<T>&, const passthrough_allocator<U>&) {
    return false;
}
  
template<typename T> class readview_dispatcher {
public:
  void operator()(Datastore *store, uint idx, void **raw_channels, int flags = 0, float scale = 1.0)
  {
    //cast
    std::vector<MultiArrayView<2, T>> **channels = reinterpret_cast<std::vector<MultiArrayView<2, T>> **>(raw_channels);
    
    //allocate correct type
    if (*channels == NULL)
      *channels = new std::vector<vigra::MultiArrayView<2, T>>(store->channels());
    
    //read
    std::vector<cv::Mat> cv_channels;
    readCvMat(store, idx, cv_channels, flags, scale);
    
    //store in multiarrayview
    for(int c=0;c<(*channels)->size();c++)
      (**channels)[c] = vigra::MultiArrayView<2, T>(imgShape(store), (T*)cv_channels[c].data);
  }
};

void readView(Datastore *store, uint idx, void **channels, int flags, float scale)
{
  store->call<readview_dispatcher>(store, idx, channels, flags, scale);
}

}