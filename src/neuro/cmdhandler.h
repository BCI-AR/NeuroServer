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

/* TODO: fix me, redefine / refactor */
int fetchIntParameters(struct CommandHandler *ch, int *vals, int num);

#endif
