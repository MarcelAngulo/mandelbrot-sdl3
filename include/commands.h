
#ifndef COMMANDS_H
#define COMMANDS_H

#include "app.h"

bool parse_arguments(AppState *app, int argc, char *argv[]);
bool execute_command(AppState *app, int argc, char *argv[]);

#endif
