#ifndef __CMDHANDLER_H
#define __CMDHANDLER_H

struct CommandHandler;

struct CommandHandler *newCommandHandler(void);
void freeCommandHandler(struct CommandHandler *ch);

/* Call this when you accept a new client */
void newClientStarted(struct CommandHandler *ch, int cliIndex);

/* Pass in command string and function pointer */
void enregisterCommand(struct CommandHandler *ch, const char *cmd, void (*func)(struct CommandHandler *ch, int cliIndex));

/* Handles characters using the command handler state machine */
void handleLine(struct CommandHandler *ch, const char *line, int cliIndex);

/* Gets the integer parameters associated with the last line parsed */
/* This will skip the command name itself.  Fetches num of them */
/* Returns results in val.  Returns 0 to indicate success */
int fetchIntParameters(struct CommandHandler *ch, int *vals, int num);

/* Gets just the command name */
const char *fetchCommandName(struct CommandHandler *ch);

/* Gets a single string parameter */
const char *fetchStringParameter(struct CommandHandler *ch, int which);

#endif
