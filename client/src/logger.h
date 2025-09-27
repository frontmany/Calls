#pragma once
#include <iostream>

#ifdef DEBUG
#define DEBUG_LOG(message) std::cout << message << std::endl;
#else
#define DEBUG_LOG(message) 
#endif