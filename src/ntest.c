#include <neuro/neuro.h>
#include <unistd.h>
#include <neuro/ns2net.h>
#include <string.h>

struct NSCounter { int success, timedOut, refused, unknownHost; };

static void NSCHUnknownHost(void *udata) {
  struct NSCounter *count = (struct NSCounter *) udata;
  count->unknownHost += 1;
}

static void NSCHTimedOut(void *udata) {
  struct NSCounter *count = (struct NSCounter *) udata;
  count->timedOut += 1;
}

static void NSCHRefused(void *udata) {
  struct NSCounter *count = (struct NSCounter *) udata;
  count->refused += 1;
}

static void NSCHSuccess(void *udata) {
  struct NSCounter *count = (struct NSCounter *) udata;
  count->success += 1;
}

static void testNSConnect(void) {
  struct NSCounter basic = { 0, 0, 0 };
  struct NSCounter cur;
  struct NSNetConnectionHandler nsch;
  struct NSNet *ns;
  nsch.success = NSCHSuccess;
  nsch.timedOut = NSCHTimedOut;
  nsch.refused = NSCHRefused;
  nsch.unknownHost = NSCHUnknownHost;
  cur = basic;
  ns = newNSNet();
  attemptConnect(ns, &nsch, "notAValidHostName", 5555, &cur);
  rassert(cur.success == 0 && cur.unknownHost == 1 && cur.refused == 0 && cur.timedOut == 0);
  cur = basic;
  rassert(attemptConnect(ns, &nsch, "localhost", 5555, &cur));
  waitForNetEvent(ns, 2000);
  rassert(cur.success == 0 && cur.unknownHost == 0 && cur.refused == 1 && cur.timedOut == 0);
  cur = basic;
  rassert(attemptConnect(ns, &nsch, "localhost", 22, &cur));
  waitForNetEvent(ns, 1000);
  rassert(cur.success == 1 && cur.unknownHost == 0 && cur.refused == 0 && cur.timedOut == 0);
}

static void testNSNet(void)
{
  testNSConnect();
  if (fork()) {
  }
  else {
  }
}

static void testStringTable(void)
{
  int val;
  struct StringTable *st;
  st = newStringTable();
  rassert(st);
  rassert(putString(st, "cat", &val) == 0);
  rassert(findString(st, "cat") == &val);
  rassert(findString(st, "dog") == NULL);
  rassert(delString(st, "dog") == ERR_NOSTRING);
  rassert(delString(st, "cat") == 0);
  rassert(findString(st, "cat") == NULL);
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

static void testCommandHandler(void)
{
  struct CommandHandler *ch;
  gotUnknown = 0;
  gotParm = NULL;
  ch = newCommandHandler();
  newClientStarted(ch, 3);
  handleLine(ch, "what", 3);
  enregisterCommand(ch, "unknown", handleUnknown);
  enregisterCommand(ch, "print", handlePrint);
  rassert(gotUnknown == 0);
  handleLine(ch, "what", 3);
  rassert(gotUnknown == 1);
  handleLine(ch, "print 0", 3);
  rassert(strcmp(gotParm, "0") == 0);
  rassert(gotInt == 0);
  handleLine(ch, "print \"\"", 3);
  rassert(strcmp(gotParm, "") == 0);
  rassert(gotInt == 0);
  handleLine(ch, "print \\\"", 3);
  rassert(strcmp(gotParm, "\"") == 0);
  rassert(gotInt == 0);
  handleLine(ch, "print 87", 3);
  rassert(gotInt == 87);
  rassert(strcmp(gotParm, "87") == 0);
  rassert(strcmp(gotParm, "87") == 0);
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
  testNSNet();
  return 0;
}
