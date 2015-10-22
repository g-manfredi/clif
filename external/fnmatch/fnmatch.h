#ifndef _CLIF_FNMATCH_WIN_H
#define _CLIF_FNMATCH_WIN_H

#define FNM_PATHNAME 0


#ifdef __cplusplus
extern "C"
{
#endif

__declspec(dllexport) int fnmatch(const char *pattern, const char *string, int flags);

#ifdef __cplusplus
}
#endif

#endif