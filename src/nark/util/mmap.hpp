#ifndef __nark_util_mmap_hpp__
#define __nark_util_mmap_hpp__

#include <stddef.h>
#include "../config.hpp"

namespace nark {

FEBIRD_DLL_EXPORT void  mmap_close(void* base, size_t size);
FEBIRD_DLL_EXPORT void* mmap_load(const char* fname, size_t* size, bool writable = false);

template<class String>
void* mmap_load(const String& fname, size_t* size, bool writable = false) {
	return mmap_load(fname.c_str(), size, writable);
}

} // namespace nark

#endif // __nark_util_mmap_hpp__

