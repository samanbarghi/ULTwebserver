#include <string.h>

#include "utserver_memory.h"

utserver_memory utserver_memory_ref(const char *base, size_t size)
{
  return (utserver_memory){base, size};
}

utserver_memory utserver_memory_str(const char *str)
{
  return utserver_memory_ref(str, strlen(str));
}

const char *utserver_memory_base(utserver_memory m)
{
  return m.base;
}

size_t utserver_memory_size(utserver_memory m)
{
  return m.size;
}

int utserver_memory_empty(utserver_memory m)
{
  return utserver_memory_base(m) == NULL;
}

int utserver_memory_equal(utserver_memory m1, utserver_memory m2)
{
  return utserver_memory_size(m1) == utserver_memory_size(m2) && memcmp(utserver_memory_base(m1), utserver_memory_base(m2), utserver_memory_size(m1)) == 0;
}

int utserver_memory_equal_case(utserver_memory m1, utserver_memory m2)
{
  return utserver_memory_size(m1) == utserver_memory_size(m2) && strncasecmp(utserver_memory_base(m1), utserver_memory_base(m2), utserver_memory_size(m1)) == 0;
}
