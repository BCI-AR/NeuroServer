#include <neuro/neuro.h>
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

static void handlePrint(struct CommandHandler *ch, int cliIndex) {
  int val;
  fetchIntParameters(ch, &val, 1);
  printf("Got parameter %d\n", val);
}

static void handleUnknown(struct CommandHandler *ch, int cliIndex) {
  printf("Unknown command for client %d\n", cliIndex);
}

void testCommandHandler(void)
{
  struct CommandHandler *ch;
  ch = newCommandHandler();
  newClientStarted(ch, 3);
  handleLine(ch, "what", 3);
  enregisterCommand(ch, "unknown", handleUnknown);
  enregisterCommand(ch, "print", handlePrint);
  handleLine(ch, "what", 3);
  handleLine(ch, "print 87", 3);
  handleLine(ch, "print \"45\"", 3);
  handleLine(ch, "print \"48\" ", 3);
  handleLine(ch, "print    \"14\" ", 3);
  handleLine(ch, "print   11  ", 3);
  freeCommandHandler(ch);
}

int main(int argc, char **argv)
{
  testStringTable();
  testCommandHandler();
  return 0;
}
