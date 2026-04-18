#include "chunk.h"

chunk_t chunks[NUM_CHUNK_DIVISIONS][NUM_CHUNK_DIVISIONS];

float world_x_min;
float world_x_max;
float world_z_min;
float world_z_max;

/*
            /----/----/----/----/               /----/
           /    /    /    /    /      .        /    / 
          /----/----/----/----/      .        /----/
         /    /    /    /    /      .        /    /
        /----/----/----/----/      .        /----/
       /    /    /    /    /      .        /    /           10x10
      /----/----/----/----/      .        /----/
                              .....      
    /----/----/----/----/      .        /----/
   /    /    /    /    /      .        /    /
  /----/----/----/----/               /----/

LOAD/BUILD STAGE:
  world built of chunks. but only static meshes are chunked.
  after loading all objects, we find the world XZ boundaries.
  then form 10x10 chunks by dividing the XZ plane into 10x10 chunks.
  initialize each chunk's xmin/xmax zmin/zmax to form chunk boundaries
  set chunk's empty = true
  loop through all static objects, and for each of the object's faces:
    use transformed world coordinates (these were pre-computed when loading object)
    compute AABB for each face (using the transformed vertices)
    test overlap with each chunk and add face/object to chunk
    NOTE: compute the chunk the face possibly overlaps with by using:
      cx_min = (tri_x_min - world_x_min) / chunk_size;
      cx_max = (tri_x_max - world_x_min) / chunk_size;

      cz_min = ...
      cz_max = ...

      this gives a smaller range of chunks we loop through to test against, instead of looping thru all chunks.
  if there is overlap:  
    add object_id, and face index to chunk
    update/expand chunk's ymin/ymax based on that face 
    mark chunk non empty

RENDER STAGE
  Render sparse/dynamic objects the normal way.
  For each non-empty chunk:
  classify chunk AABB vs frustum as:
    outside
    intersecting
    inside
    If outside: skip.
    If intersecting:
    render stored faces with normal triangle clipping.
    If inside:
    render stored faces without triangle clipping.
    Use a per-frame face stamp to avoid drawing duplicated overlap faces more than once.

*/

/* go through all static/chunkable objects in the world, for each object, get the face vertices
   and keep track of minimum and maximum x, and z.  use those as the world bounds */
void calculate_world_bounds(void){
  float x_min = 0.0f, x_max = 0.0f;
  float z_min = 0.0f, z_max = 0.0f;

  for(uint32_t i = 0; i < num_objects; i++){
    object_t *object = &objects[i];
    uint32_t num_vertices = object->mesh->num_vertices;

    if(object->fixed){
      for(uint32_t vertex_i = 0; vertex_i < num_vertices; vertex_i++){
        vector3_t vertex = object->transformed_vertices[vertex_i];

        if(vertex.x < x_min) x_min = vertex.x;
        if(vertex.x > x_max) x_max = vertex.x;
        if(vertex.z < z_min) z_min = vertex.z;
        if(vertex.z > z_max) z_max = vertex.z;
      }
    }
  }

  /* set world boundaries */
  world_x_min = x_min;
  world_x_max = x_max;
  world_z_min = z_min;
  world_z_max = z_max;
  

  printf("\nxmin: %f, xmax: %f\n", x_min, x_max);
  printf("\nzmin: %f, zmax: %f\n", z_min, z_max);
}