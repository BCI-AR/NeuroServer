#include <neuro/stringtable.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

#define INITSIZE 10

struct StringTableNode {
  char *key;
  void *val;
};

struct StringTable {
  int allocSize, usedSize;
  int freeHoles;
  struct StringTableNode *tab;
};

struct StringTable *newStringTable(void)
{
  struct StringTable *st;
  st = calloc(sizeof(struct StringTable), 1);
  st->allocSize = INITSIZE;
  st->usedSize = 0;
  st->freeHoles = 0;
  st->tab = calloc(sizeof(struct StringTableNode), st->allocSize);
  return st;
}

static int findIndex(struct StringTable *st, const char *key)
{
  int i;
  for (i = 0; i < st->usedSize; i += 1) {
    if (st->tab[i].key == NULL) continue;
    if (strcmp(st->tab[i].key, key) == 0)
      return i;
  }
  return -1;
}

static int getFreeHoleIndex(struct StringTable *st)
{
  int i;
  assert(st->freeHoles > 0);
  for (i = 0; i < st->usedSize; ++i) {
    if (st->tab[i].key == NULL) {
      st->freeHoles -= 1;
      return i;
    }
  }
  assert(0 && "StringTable free hole error.");
  return -1;
}

static int getFreeIndex(struct StringTable *st)
{
  if (st->freeHoles)
    return getFreeHoleIndex(st);
  if (st->usedSize < st->allocSize) {
    st->usedSize += 1;
    return st->usedSize - 1;
  }
  st->allocSize *= 2;
  st->tab = realloc(st->tab, sizeof(struct StringTableNode)*st->allocSize);
  return getFreeIndex(st);
}

int putString(struct StringTable *st, const char *key, void *val)
{
  int ind = findIndex(st, key);
  if (ind == -1) {
    ind = getFreeIndex(st);
    st->tab[ind].key = strdup(key);
  }
  st->tab[ind].val = val;
  return 0;
}

int delString(struct StringTable *st, const char *key)
{
  int ind = findIndex(st, key);
  if (ind == -1) return ERR_NOSTRING;
  free(st->tab[ind].key);
  st->tab[ind].key = NULL;
  st->freeHoles += 1;
  return 0;
}

void *findString(struct StringTable *st, const char *key)
{
  int ind = findIndex(st, key);
  return (ind == -1) ? NULL : st->tab[ind].val;
}

void allStrings(struct StringTable *st, StringTableIterator sti, void *udata)
{
  int i;
  for (i = 0; i < st->usedSize; i += 1) {
    if (st->tab[i].key != NULL)
      sti(st, st->tab[i].key, st->tab[i].val, udata);
  }
}

void freeStringTable(struct StringTable *st)
{
  int i;
  for (i = 0; i < st->usedSize; i += 1) {
    if (st->tab[i].key != NULL) {
      free(st->tab[i].key);
      st->tab[i].key = NULL;
    }
  }
  free(st->tab);
  free(st);
}

