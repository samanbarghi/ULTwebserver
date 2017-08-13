#ifndef UTSERVER_MEMORY_H_INCLUDED
#define UTSERVER_MEMORY_H_INCLUDED

typedef struct utserver_memory utserver_memory;
struct utserver_memory
{
  const char *base;
  size_t      size;

utserver_memory  utserver_memory_ref(const char *, size_t);
utserver_memory  utserver_memory_str(const char *);
const char     *utserver_memory_base(utserver_memory);
size_t          utserver_memory_size(utserver_memory);
int             utserver_memory_empty(utserver_memory);
int             utserver_memory_equal(utserver_memory, utserver_memory);
int             utserver_memory_equal_case(utserver_memory, utserver_memory);


#endif /* UTSERVER_MEMORY_H_INCLUDED */

