#pragma once
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <chrono>

// Prevent Windows.h from defining min/max macros
#define NOMINMAX

#include <winsock2.h>   // must be included before ws2tcpip.h / windows.h

// Windows headers for system monitoring
#include <windows.h>
#include <Pdh.h>

#include "dds/dds.h"
#include "SharedData.h"
#include "NetworkDefs.h"