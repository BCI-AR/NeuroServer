#include <neuro/neuro.h>
#include <string.h>
#include <assert.h>

void testStringTable(void)
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
}

static const char *gotParm;
static int gotInt;
static int gotUnknown;

static void handlePrint(struct CommandHandler *ch, int cliIndex) {
  gotParm = fetchStringParameter(ch, 0);
  fetchIntParameters(ch, &gotInt, 1);
}

static void handleUnknown(struct CommandHandler *ch, int cliIndex) {
  gotUnknown = 1;
}

void testCommandHandler(void)
{
  struct CommandHandler *ch;
  gotUnknown = 0;
  gotParm = NULL;
  ch = newCommandHandler();
  newClientStarted(ch, 3);
  handleLine(ch, "what", 3);
  enregisterCommand(ch, "unknown", handleUnknown);
  enregisterCommand(ch, "print", handlePrint);
  assert(gotUnknown == 0);
  handleLine(ch, "what", 3);
  assert(gotUnknown == 1);
  handleLine(ch, "print 0", 3);
  assert(strcmp(gotParm, "0") == 0);
  assert(gotInt == 0);
  handleLine(ch, "print \"\"", 3);
  assert(strcmp(gotParm, "") == 0);
  assert(gotInt == 0);
  handleLine(ch, "print \\\"", 3);
  assert(strcmp(gotParm, "\"") == 0);
  assert(gotInt == 0);
  handleLine(ch, "print 87", 3);
  assert(gotInt == 87);
  assert(strcmp(gotParm, "87") == 0);
  assert(strcmp(gotParm, "87") == 0);
  handleLine(ch, "print \"45\"", 3);
  handleLine(ch, "print \"48\" ", 3);
  handleLine(ch, "print    \"14\" ", 3);
  handleLine(ch, "print   11  ", 3);
  handleLine(ch, "print \"hi\"", 3);
  handleLine(ch, "print \" hi\"", 3);
  handleLine(ch, "print \"h i\"", 3);
  handleLine(ch, "print \"hi \"", 3);
  handleLine(ch, "print \"h\\\\i\"", 3);
  handleLine(ch, "print \"h\\\"i\"", 3);
  freeCommandHandler(ch);
}

int main(int argc, char **argv)
{
  testStringTable();
  testCommandHandler();
  return 0;
}
