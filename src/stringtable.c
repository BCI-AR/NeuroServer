#include <neuro/stringtable.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

/* To get tdestroy prototype */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <search.h>

struct StringTableNode {
  char *key;
  void *val;
};

struct StringTable {
  void *stringTableNodeTab;
};

struct StringTable *newStringTable(void)
{
  struct StringTable *st;
  st = calloc(sizeof(struct StringTable), 1);
  return st;
}

static const char *makeStringFromInt(int key)
{
  static char buf[16];
  sprintf(buf, "%d", key);
  return buf;
}

int stringToInt(const char *str)
{
  int result;
  sscanf(str, "%d", &result);
  return result;
}

static int compare(const void *a, const void *b)
{
  const char *ca = *(const char **) a, *cb = * (const char **) b;
  return strcmp(ca, cb);
}

int putInt(struct StringTable *st, int key, void *val)
{
  return putString(st, makeStringFromInt(key), val);
}

int putString(struct StringTable *st, const char *key, void *val)
{
  struct StringTableNode *result;
  result = (struct StringTableNode *)
           tfind(&key, &st->stringTableNodeTab, compare);
  if (result == NULL) {
    result = calloc(sizeof(*result), 1);
    result->key = strdup(key);
    result->val = val;
    tsearch(result, &st->stringTableNodeTab, compare);
  }
  else {
    result = * (struct StringTableNode **) result;
    result->val = val;
  }
  return 0;
}

int delInt(struct StringTable *st, int key)
{
  return delString(st, makeStringFromInt(key));
}

int delString(struct StringTable *st, const char *key)
{
  struct StringTableNode *result;
  char *toFree;
  result = (struct StringTableNode *)
           tfind(&key, &st->stringTableNodeTab, compare);
  if (result == NULL) return ERR_NOSTRING;
  result = * (struct StringTableNode **) result;
  toFree = result->key;
  tdelete(result, &st->stringTableNodeTab, compare);
  free(toFree);
  free(result);
  return 0;
}

void *findInt(struct StringTable *st, int key)
{
  return findString(st, makeStringFromInt(key));
}

void *findString(struct StringTable *st, const char *key)
{
  struct StringTableNode **result;
  result = (struct StringTableNode **)
           tfind(&key, &st->stringTableNodeTab, compare);
  return (result == NULL) ? NULL : (*result)->val;
}

static void *audata;
static StringTableIterator asti;
static struct StringTable *ast;

static void allFunc(const void *nodep, const VISIT which, const int depth)
{
  struct StringTableNode *result;
  if (which != endorder && which != leaf) return;
  result = *(struct StringTableNode **) nodep;
  asti(ast, result->key, result->val, audata);
}

static void freeFunc(void *nodep)
{
  struct StringTableNode *result;
  result = (struct StringTableNode *) nodep;
  free(result->key);
  free(result);
}


void allStrings(struct StringTable *st, StringTableIterator sti, void *udata)
{
  audata = udata;
  asti = sti;
  ast = st;
  twalk(st->stringTableNodeTab, allFunc);
}

void freeStringTable(struct StringTable *st)
{
  tdestroy(st->stringTableNodeTab, freeFunc);
  st->stringTableNodeTab = NULL;
  free(st);
}

