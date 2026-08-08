
#ifndef FILE_IO_H
#define FILE_IO_H

#include "app.h"

void save_image(AppState *app, char *fn);
void save_status(AppState *app, char *fn);
void read_status(AppState *app, char *fn);

#endif
