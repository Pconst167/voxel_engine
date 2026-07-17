#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>
#include "parallel_renderer.h"
#include "main.h"
#include "display.h"
#include "log.h"

#define MAX_RENDER_WORKER_THREADS 32
#define INITIAL_TRIANGLES_PER_BAND (4096 * 2)

typedef struct{
  SDL_Thread *thread;
  triangle_t *triangles;
  uint32_t num_triangles;
  uint32_t triangle_capacity;
  uint64_t pixels_rendered;
  int32_t worker_index;
  int32_t y_min;
  int32_t y_max;
  uint32_t completed_generation;
} render_worker_t;

typedef struct{
  render_worker_t workers[MAX_RENDER_WORKER_THREADS];
  SDL_mutex *mutex;
  SDL_cond *work_available_condition;
  SDL_cond *work_complete_condition;
  uint32_t work_generation;
  int32_t workers_remaining;
  int32_t worker_count;
  bool initialized;
  bool shutting_down;
} parallel_renderer_t;

static parallel_renderer_t parallel_renderer;

static bool grow_worker_triangle_array(render_worker_t *worker, uint32_t required_capacity){
  if(required_capacity <= worker->triangle_capacity) return true;

  uint32_t new_capacity = worker->triangle_capacity > 0 ? worker->triangle_capacity : INITIAL_TRIANGLES_PER_BAND;
  while(new_capacity < required_capacity){
    if(new_capacity > UINT32_MAX / 2) return false;
    new_capacity *= 2;
  }

  triangle_t *new_triangles = realloc(worker->triangles, sizeof(triangle_t) * new_capacity);
  if(new_triangles == NULL) return false;

  worker->triangles = new_triangles;
  worker->triangle_capacity = new_capacity;
  return true;
}

static int render_worker_main(void *data){
  render_worker_t *worker = data;
  rasterizer_set_y_bounds(worker->y_min, worker->y_max);

  SDL_LockMutex(parallel_renderer.mutex);
  while(!parallel_renderer.shutting_down){
    while(!parallel_renderer.shutting_down && worker->completed_generation == parallel_renderer.work_generation)
      SDL_CondWait(parallel_renderer.work_available_condition, parallel_renderer.mutex);

    if(parallel_renderer.shutting_down) break;

    uint32_t work_generation = parallel_renderer.work_generation;
    uint32_t num_triangles = worker->num_triangles;
    triangle_t *triangles = worker->triangles;
    SDL_UnlockMutex(parallel_renderer.mutex);

    uint64_t pixels_rendered = 0;
    for(uint32_t triangle_index = 0; triangle_index < num_triangles; triangle_index++)
      pixels_rendered += rasterize_triangle(&triangles[triangle_index]);

    SDL_LockMutex(parallel_renderer.mutex);
    worker->pixels_rendered = pixels_rendered;
    worker->completed_generation = work_generation;
    parallel_renderer.workers_remaining--;
    if(parallel_renderer.workers_remaining == 0)
      SDL_CondSignal(parallel_renderer.work_complete_condition);
  }
  SDL_UnlockMutex(parallel_renderer.mutex);

  rasterizer_reset_y_bounds();
  return 0;
}

bool parallel_renderer_init(void){
  if(parallel_renderer.initialized) return true;
  if(!rasterizer_initialize_thread_local_storage()) return false;

  int32_t cpu_count = SDL_GetCPUCount();
  if(cpu_count < 1) cpu_count = 1;
  if(cpu_count > MAX_RENDER_WORKER_THREADS) cpu_count = MAX_RENDER_WORKER_THREADS;
  if(cpu_count > window_height) cpu_count = window_height;

  parallel_renderer.mutex = SDL_CreateMutex();
  parallel_renderer.work_available_condition = SDL_CreateCond();
  parallel_renderer.work_complete_condition = SDL_CreateCond();
  if(parallel_renderer.mutex == NULL || parallel_renderer.work_available_condition == NULL || parallel_renderer.work_complete_condition == NULL){
    parallel_renderer_shutdown();
    return false;
  }

  parallel_renderer.worker_count = cpu_count;
  parallel_renderer.work_generation = 0;
  parallel_renderer.shutting_down = false;

  for(int32_t worker_index = 0; worker_index < parallel_renderer.worker_count; worker_index++){
    render_worker_t *worker = &parallel_renderer.workers[worker_index];
    worker->worker_index = worker_index;
    worker->y_min = worker_index * window_height / parallel_renderer.worker_count;
    worker->y_max = ((worker_index + 1) * window_height / parallel_renderer.worker_count) - 1;
    worker->completed_generation = 0;

    char thread_name[32];
    snprintf(thread_name, sizeof(thread_name), "raster-band-%d", worker_index);
    worker->thread = SDL_CreateThread(render_worker_main, thread_name, worker);
    if(worker->thread == NULL){
      parallel_renderer.worker_count = worker_index;
      parallel_renderer_shutdown();
      return false;
    }
  }

  parallel_renderer.initialized = true;
  print_log("Parallel rasterizer initialized with %d screen-band workers", parallel_renderer.worker_count);
  return true;
}

void parallel_renderer_shutdown(void){
  if(parallel_renderer.mutex != NULL){
    SDL_LockMutex(parallel_renderer.mutex);
    parallel_renderer.shutting_down = true;
    if(parallel_renderer.work_available_condition != NULL)
      SDL_CondBroadcast(parallel_renderer.work_available_condition);
    SDL_UnlockMutex(parallel_renderer.mutex);
  }

  for(int32_t worker_index = 0; worker_index < parallel_renderer.worker_count; worker_index++){
    render_worker_t *worker = &parallel_renderer.workers[worker_index];
    if(worker->thread != NULL){
      SDL_WaitThread(worker->thread, NULL);
      worker->thread = NULL;
    }
    free(worker->triangles);
    worker->triangles = NULL;
    worker->num_triangles = 0;
    worker->triangle_capacity = 0;
  }

  if(parallel_renderer.work_complete_condition != NULL){
    SDL_DestroyCond(parallel_renderer.work_complete_condition);
    parallel_renderer.work_complete_condition = NULL;
  }
  if(parallel_renderer.work_available_condition != NULL){
    SDL_DestroyCond(parallel_renderer.work_available_condition);
    parallel_renderer.work_available_condition = NULL;
  }
  if(parallel_renderer.mutex != NULL){
    SDL_DestroyMutex(parallel_renderer.mutex);
    parallel_renderer.mutex = NULL;
  }

  parallel_renderer.worker_count = 0;
  parallel_renderer.workers_remaining = 0;
  parallel_renderer.work_generation = 0;
  parallel_renderer.initialized = false;
  parallel_renderer.shutting_down = false;
}

bool parallel_renderer_is_available(void){
  return parallel_renderer.initialized && parallel_renderer.worker_count > 1;
}

int32_t parallel_renderer_get_worker_count(void){
  return parallel_renderer.worker_count;
}

void parallel_renderer_begin_batch(void){
  for(int32_t worker_index = 0; worker_index < parallel_renderer.worker_count; worker_index++){
    parallel_renderer.workers[worker_index].num_triangles = 0;
    parallel_renderer.workers[worker_index].pixels_rendered = 0;
  }
}

bool parallel_renderer_submit_triangle(const triangle_t *triangle){
  if(!parallel_renderer.initialized || parallel_renderer.worker_count <= 0) return false;

  float triangle_y_min = triangle->points[0].y;
  float triangle_y_max = triangle->points[0].y;
  for(int32_t vertex_index = 1; vertex_index < 3; vertex_index++){
    if(triangle->points[vertex_index].y < triangle_y_min) triangle_y_min = triangle->points[vertex_index].y;
    if(triangle->points[vertex_index].y > triangle_y_max) triangle_y_max = triangle->points[vertex_index].y;
  }

  int32_t y_min = (int32_t)floorf(triangle_y_min);
  int32_t y_max = (int32_t)ceilf(triangle_y_max);
  if(y_max < 0 || y_min >= window_height) return true;
  if(y_min < 0) y_min = 0;
  if(y_max >= window_height) y_max = window_height - 1;

  int32_t first_worker_index = y_min * parallel_renderer.worker_count / window_height;
  int32_t last_worker_index = y_max * parallel_renderer.worker_count / window_height;
  if(first_worker_index < 0) first_worker_index = 0;
  if(last_worker_index >= parallel_renderer.worker_count) last_worker_index = parallel_renderer.worker_count - 1;

  for(int32_t worker_index = first_worker_index; worker_index <= last_worker_index; worker_index++){
    render_worker_t *worker = &parallel_renderer.workers[worker_index];
    if(!grow_worker_triangle_array(worker, worker->num_triangles + 1)) return false;
  }

  for(int32_t worker_index = first_worker_index; worker_index <= last_worker_index; worker_index++){
    render_worker_t *worker = &parallel_renderer.workers[worker_index];
    worker->triangles[worker->num_triangles] = *triangle;
    worker->num_triangles++;
  }

  return true;
}

uint32_t parallel_renderer_flush(void){
  if(!parallel_renderer.initialized || parallel_renderer.worker_count <= 0) return 0;

  SDL_LockMutex(parallel_renderer.mutex);
  parallel_renderer.workers_remaining = parallel_renderer.worker_count;
  parallel_renderer.work_generation++;
  SDL_CondBroadcast(parallel_renderer.work_available_condition);

  while(parallel_renderer.workers_remaining > 0)
    SDL_CondWait(parallel_renderer.work_complete_condition, parallel_renderer.mutex);
  SDL_UnlockMutex(parallel_renderer.mutex);

  uint64_t total_pixels_rendered = 0;
  for(int32_t worker_index = 0; worker_index < parallel_renderer.worker_count; worker_index++)
    total_pixels_rendered += parallel_renderer.workers[worker_index].pixels_rendered;

  if(total_pixels_rendered > UINT32_MAX) total_pixels_rendered = UINT32_MAX;
  return (uint32_t)total_pixels_rendered;
}
