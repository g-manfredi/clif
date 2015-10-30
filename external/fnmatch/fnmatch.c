#include "fnmatch.h"

#include <Shlwapi.h>

__declspec(dllexport) int fnmatch(const char *pattern, const char *string, int flags)
{
	if (((flags & FNM_PATHNAME) != FNM_PATHNAME) || (flags & FNM_CASEFOLD) || (flags & FNM_EXTMATCH)) {
		printf("FIXME implement workaround for missing features in PathMatchSpec!\n");
		abort();
	}

	if (PathMatchSpec(string, pattern))
		return 0;
	return -1;
}