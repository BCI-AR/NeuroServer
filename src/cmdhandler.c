#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <neuro/stringtable.h>
#include <neuro/cmdhandler.h>
#include "nsutil.h"
#include "nsnet.h"
#include "nsd.h"

#define MAXTOKENS 256

struct CommandHandler {
  struct StringTable *cmds, *clients;
  struct ClientState *curClient;
};

struct CommandEntry {
	void (*func)(struct CommandHandler *ch, int cliInd);
};

struct ClientState {
	int newLineStarted;
  char *curline;
  int tokenCount;
  char *tokens[MAXTOKENS];
};

struct CommandHandler *newCommandHandler(void)
{
  struct CommandHandler *ch;
  ch = calloc(sizeof(struct CommandHandler), 1);
  ch->cmds = newStringTable();
  ch->clients = newStringTable();
  return ch;
}

void freeValues(struct StringTable *st, const char *key, void *val, void *udata)
{
  free(val);
}

void freeCS(struct StringTable *st, const char *key, void *val, void *udata)
{
  struct ClientState *cs = (struct ClientState *) val;
  if (cs->curline != NULL) {
    free(cs->curline);
    cs->curline = NULL;
  }
}

void freeCommandHandler(struct CommandHandler *ch)
{
  allStrings(ch->cmds, freeValues, NULL);
  allStrings(ch->clients, freeCS, NULL);
  allStrings(ch->clients, freeValues, NULL);
  freeStringTable(ch->cmds);
  freeStringTable(ch->clients);
  ch->cmds = ch->clients = NULL;
  free(ch);
}

static const char *indexToStr(int cliIndex)
{
  static char buf[80];
  sprintf(buf, "%d", cliIndex);
  return buf;
}

/* Call this when you accept a new client */
void newClientStarted(struct CommandHandler *ch, int cliIndex)
{
  struct ClientState *cs;
  cs = calloc(sizeof(struct ClientState), 1);
  putString(ch->clients, indexToStr(cliIndex), cs);
}

/* Pass in command string and function pointer */
void enregisterCommand(struct CommandHandler *ch, const char *cmd, void (*func)(struct CommandHandler *ch, int))
{
  struct CommandEntry *ce;
  ce = calloc(sizeof(struct CommandEntry), 1);
  ce->func = func;
  putString(ch->cmds, cmd, ce);
}

static void replaceCurline(struct ClientState *cs, const char *line)
{
  int linelen = strlen(line);
  if (cs->curline)
    free(cs->curline);
  cs->curline = calloc(strlen(line)+2, 1);
  memcpy(cs->curline, line, linelen);
  cs->curline[linelen] = ' ';
  cs->curline[linelen+1] = 0;
}

static void addToken(struct ClientState *cs, char *tok)
{
  int i, j=0;
  assert(cs->tokenCount < MAXTOKENS);
  for (i = 0; tok[i]; i += 1) {
    if (tok[i] == '\\' && tok[i+1])
      i += 1;
    tok[j] = tok[i];
    j += 1;
  }
  tok[j] = 0;
  cs->tokens[cs->tokenCount] = tok;
  cs->tokenCount += 1;
}

static void tokenizeLine(struct ClientState *cs)
{
  int i;
  int inQuote=0;
  int inBackslash=0;
  int inSpace=1;
  int startPos = 0;
  memset(cs->tokens, 0, sizeof(cs->tokens));
  cs->tokenCount = 0;
  for (i = 0; cs->curline[i]; i += 1) {
    char c = cs->curline[i];
    if (inBackslash) {
      inBackslash = 0;
      continue;
    }
    if (c == '\\') {
      inBackslash = 1;
      continue;
    }
    if (c == '"') {
      if (inQuote) {
        inQuote = 0;
        inSpace = 1;
        cs->curline[i] = 0;
        addToken(cs, cs->curline+startPos);
        continue;
      } else {
        if (cs->curline[i+1]) {
          startPos = i+1;
          inSpace = 0;
          inQuote = 1;
        }
        continue;
      }
    }
    if (c == '\r' || c == '\n' || c == '\t' || c == ' '
                  || cs->curline[i+1] == 0) {
      if (inQuote) continue;
      if (inSpace) continue;
      if (c == '\r' || c == '\n' || c == '\t' || c == ' ')
        cs->curline[i] = 0;
      addToken(cs, cs->curline+startPos);
      inSpace = 1;
      continue;
    }
    if (inSpace) {
      startPos = i;
      inSpace = 0;
      continue;
    }
  }
}

void handleLine(struct CommandHandler *ch, const char *line, int cliIndex)
{
  struct ClientState *cs;
  struct CommandEntry *ce;
  cs = findString(ch->clients, indexToStr(cliIndex));
  assert(cs != NULL);
  replaceCurline(cs, line);
  tokenizeLine(cs);
  if (0)
  { int i;
    for (i = 0; i < cs->tokenCount; ++i)
      printf("%d: <%s>\n", i, cs->tokens[i]);
  }
  ce = findString(ch->cmds, cs->tokens[0]);
  if (ce == NULL)
    ce = findString(ch->cmds, "unknown");
  if (ce) {
    ch->curClient = cs;
    ce->func(ch, cliIndex);
    ch->curClient = NULL;
  }
}

/* TODO: fix me, redefine / refactor */
int fetchIntParameters(struct CommandHandler *ch, int *vals, int num)
{
  int i;
  for (i = 0; i < num; ++i)
    vals[i] = atoi(ch->curClient->tokens[i+1]);
  return 0;
}

