#ifndef __STRINGTABLE_H
#define __STRINGTABLE_H

#include <stdio.h>

struct StringTable;

#define ERR_NOSTRING 1
#define ERR_INVALID  2
#define ERR_TABLEFULL 3

typedef void (*StringTableIterator)
  (struct StringTable *st, const char *key, void *val, void *udata);
struct StringTable *newStringTable(void);
int putString(struct StringTable *st, const char *key, void *val);
int delString(struct StringTable *st, const char *key);
void *findString(struct StringTable *st, const char *key);
void allStrings(struct StringTable *st, StringTableIterator sti, void *udata);
void freeStringTable(struct StringTable *st);

#endif
