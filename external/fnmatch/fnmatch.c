#include "fnmatch.h"

#include <Shlwapi.h>

__declspec(dllexport) int fnmatch(const char *pattern, const char *string, int flags)
{
	if (PathMatchSpec(string, pattern))
		return 0;
	return -1;
}