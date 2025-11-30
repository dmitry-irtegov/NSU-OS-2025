#ifndef __BUILTINS_H
#define __BUILTINS_H

#include "jobs.h"
#include "pipeline.h"

int try_builtin(pipeline_t *pipeline, joblist_t *jobs, char *is_error);

#endif // __BUILTINS_H
