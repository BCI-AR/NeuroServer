#include <neuro/neuro.h>
#include <assert.h>

int main(int argc, char **argv)
{
  int val;
  struct StringTable *st;
  st = newStringTable();
  assert(st);
  assert(putString(st, "cat", &val) == 0);
  assert(findString(st, "cat") == &val);
  assert(findString(st, "dog") == NULL);
  assert(delString(st, "dog") == ERR_NOSTRING);
  assert(delString(st, "cat") == 0);
  assert(findString(st, "cat") == NULL);
  freeStringTable(st);
  return 0;
}

