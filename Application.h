#pragma once
#define DEBUG 1
#include "Ix64.h"

#include "IDebug.h"

#define global static
#define internal static
#define Assert(x) if(!(x)) __debugbreak();
typedef unsigned char byte;



#define APP 1

