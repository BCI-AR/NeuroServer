#ifndef __STRINGTABLE_H
#define __STRINGTABLE_H

#include <stdio.h>
#include <neuro/neuro.h>

struct StringTable;

#define ERR_NOSTRING 1
#define ERR_INVALID  2
#define ERR_TABLEFULL 3

typedef void (*StringTableIterator)
  (struct StringTable *st, const char *key, void *val, void *udata);
struct StringTable *newStringTable(void);
int putString(struct StringTable *st, const char *key, void *val);
int putInt(struct StringTable *st, int key, void *val);
int delString(struct StringTable *st, const char *key);
int delInt(struct StringTable *st, int key);
void *findString(struct StringTable *st, const char *key);
void *findInt(struct StringTable *st, int key);
int stringToInt(const char *str);
void allStrings(struct StringTable *st, StringTableIterator sti, void *udata);
void freeStringTable(struct StringTable *st);

#endif
