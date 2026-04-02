#ifdef  _WINDOWS_SOURCE
#ifdef  __TCAUTLListDocsMD
#define ExportedByTCAUTLListDocsMD     __declspec(dllexport)
#else
#define ExportedByTCAUTLListDocsMD     __declspec(dllimport)
#endif
#else
#define ExportedByTCAUTLListDocsMD
#endif
