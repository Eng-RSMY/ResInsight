
#pragma once

#include <iostream>
#include <cstdlib>

#define CAF_ASSERT(expr)                                             \
  do                                                                 \
  {                                                                  \
    if(!(expr))                                                      \
    {                                                                \
      std::cout << __FILE__ << ":" << __LINE__ << ": CAF_ASSERT("    \
                << #expr << ") failed" << std::endl;                 \
      std::abort();                                                  \
    }                                                                \
  } while(false)
