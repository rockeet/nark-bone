#include "mmap.hpp"
#include "autofree.hpp"
#include "throw.hpp"
#include <stdio.h>
#include <string.h>
#include <stdexcept>

#ifdef _MSC_VER
	#include <io.h>
	#include <Windows.h>
#else
	#include <sys/mman.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif
#include <fcntl.h>

#if !defined(MAP_POPULATE)
	#define  MAP_POPULATE 0
#endif

namespace nark {

void mmap_close(void* base, size_t size) {
#ifdef _MSC_VER
	::UnmapViewOfFile(base);
#else
	::munmap(base, size);
#endif
}

void* mmap_load(const char* fname, size_t* fsize, bool writable) {
	int openFlags = writable ? O_RDWR : O_RDONLY;
	int fd = ::open(fname, openFlags);
	if (fd < 0) {
		int err = errno;
		THROW_STD(logic_error, "open(fname=%s, %s) = %d(%X): %s"
			, fname, writable ? "O_RDWR" : "O_RDONLY"
			, err, err, strerror(err));
	}
#ifdef _MSC_VER
	LARGE_INTEGER lsize;
	HANDLE hFile = (HANDLE)_get_osfhandle(fd);
	if (!GetFileSizeEx(hFile, &lsize)) {
		DWORD err = GetLastError();
		::_close(fd);
		THROW_STD(logic_error, "GetFileSizeEx(fname=%s).Err=%d(%X)"
			, fname, err, err);
	}
	if (lsize.QuadPart > size_t(-1)) {
		::_close(fd);
		THROW_STD(logic_error, "fname=%s fsize=%I64u(%I64X) too large"
			, fname, lsize.QuadPart, lsize.QuadPart);
	}
	*fsize = size_t(lsize.QuadPart);
	DWORD flProtect = writable ? PAGE_READWRITE : PAGE_READONLY;
	HANDLE hMmap = CreateFileMapping(hFile, NULL, flProtect, 0, 0, NULL);
	if (NULL == hMmap) {
		DWORD err = GetLastError();
		::_close(fd);
		THROW_STD(runtime_error, "CreateFileMapping(fname=%s).Err=%d(0x%X)"
			, fname, err, err);
	}
	DWORD desiredAccess = (writable ? FILE_MAP_WRITE : 0) | FILE_MAP_READ;
	void* base = MapViewOfFile(hMmap, desiredAccess, 0, 0, 0);
	if (NULL == base) {
		DWORD err = GetLastError();
		::CloseHandle(hMmap);
		::_close(fd);
		THROW_STD(runtime_error, "MapViewOfFile(fname=%s).Err=%d(0x%X)"
			, fname, err, err);
	}
	::CloseHandle(hMmap); // close before UnmapViewOfFile is OK
	::_close(fd);
#else
	struct stat st;
	if (::fstat(fd, &st) < 0) {
		THROW_STD(logic_error, "stat(fname=%s) = %s", fname, strerror(errno));
	}
	*fsize = st.st_size;
	int flProtect = (writable ? PROT_WRITE : 0) | PROT_READ;
	void* base = ::mmap(NULL, st.st_size, flProtect, MAP_SHARED, fd, 0);
	if (MAP_FAILED == base) {
		::close(fd);
		THROW_STD(logic_error, "mmap(fname=%s, %s SHARED, size=%lld) = %s"
			, fname, writable ? "READWRITE" : "READ"
			, (long long)st.st_size, strerror(errno));
	}
	::close(fd);
#endif
	return base;
}

} // namespace nark

