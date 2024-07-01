#pragma once

#ifdef _WINDLL
#define COREAPI __declspec(dllexport)
#else
#define COREAPI __declspec(dllimport)
#endif
