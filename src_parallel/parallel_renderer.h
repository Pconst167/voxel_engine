#ifndef PARALLEL_RENDERER_H
#define PARALLEL_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include "triangle.h"

bool parallel_renderer_init(void);
void parallel_renderer_shutdown(void);
bool parallel_renderer_is_available(void);
int32_t parallel_renderer_get_worker_count(void);
void parallel_renderer_begin_batch(void);
bool parallel_renderer_submit_triangle(const triangle_t *triangle);
uint32_t parallel_renderer_flush(void);

#endif
