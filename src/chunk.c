#include "chunk.h"
#include "object.h"
#include "array.h"
#include "aabb.h"
#include "collision.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

chunk_t chunks[NUM_CHUNK_DIVISIONS][NUM_CHUNK_DIVISIONS] = {0};

double world_x_min;
double world_x_max;
double world_z_min;
double world_z_max;

double chunk_dx;
double chunk_dz;

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
    NOTE: compute the chunk that the face possibly overlaps with by using:
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
    ** Use a per-frame face stamp to avoid drawing duplicated overlap faces more than once.

    after all intersecting chunks are found, sort them front to back before rendering.
*/

/* go through all static/chunkable objects in the world, for each object, get the face vertices
   and keep track of minimum and maximum x, and z.  use those as the world bounds */
void calculate_world_bounds(void){
  double x_min = 0.0f, x_max = 0.0f;
  double z_min = 0.0f, z_max = 0.0f;

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

  printf("\nworld xmin: %f, world xmax: %f", x_min, x_max);
  printf("\nworld zmin: %f, world zmax: %f\n", z_min, z_max);
}

/* split the world into chunks */
void create_world_chunks(void){
  /* calculate chunk deltas */
  chunk_dx = (world_x_max - world_x_min) / (double)NUM_CHUNK_DIVISIONS;
  chunk_dz = (world_z_max - world_z_min) / (double)NUM_CHUNK_DIVISIONS;

  /* set the boundary of each chunk */
  for(uint32_t z_row = 0; z_row < NUM_CHUNK_DIVISIONS; z_row++){
    for(uint32_t x_col = 0; x_col < NUM_CHUNK_DIVISIONS; x_col++){
      chunks[z_row][x_col].empty = true;
      chunks[z_row][x_col].x_min = world_x_min + (x_col    ) * chunk_dx;     
      chunks[z_row][x_col].x_max = world_x_min + (x_col + 1) * chunk_dx;     
      chunks[z_row][x_col].z_min = world_z_min + (z_row    ) * chunk_dz;     
      chunks[z_row][x_col].z_max = world_z_min + (z_row + 1) * chunk_dz;     

      //printf("xstart: %f, xend: %f, zstart: %f, zend: %f\n", 
      //  chunks[z_row][x_col].x_min, 
      //  chunks[z_row][x_col].x_max, 
      //  chunks[z_row][x_col].z_min, 
      //  chunks[z_row][x_col].z_max 
      //);
    }
  }

  printf("\nchunk_dx: %f\n", chunk_dx);
  printf("chunk_dz: %f\n", chunk_dz);
}

/*
  loop through all static objects, and for each of the object's faces:
    use transformed world coordinates (these were pre-computed when loading object)
    compute AABB for each face (using the transformed vertices)
    test overlap with each chunk and add face/object to chunk
    NOTE: compute the chunk that the face possibly overlaps with by using:
      cx_min = (tri_x_min - world_x_min) / chunk_size;
      cx_max = (tri_x_max - world_x_min) / chunk_size;

      cz_min = ...
      cz_max = ...

      this gives a smaller range of chunks we loop through to test against, instead of looping thru all chunks.
  if there is overlap:  
    add object_id, and face index to chunk
    update/expand chunk's ymin/ymax based on that face 
    mark chunk non empty
*/
void populate_chunks_with_objects(void){
  for(uint32_t object_id = 0; object_id < num_objects; object_id++){
    object_t *object = &objects[object_id];
    if(object->fixed){
      for(uint32_t face_i = 0; face_i < object->mesh->num_faces; face_i++){
        face_t face = object->mesh->faces[face_i];
        /* obtain the face's 3 vertices */
        vector3_t v_a = object->transformed_vertices[face.vertex_indices[0]];
        vector3_t v_b = object->transformed_vertices[face.vertex_indices[1]];
        vector3_t v_c = object->transformed_vertices[face.vertex_indices[2]];
        /* form face's 2d AABB */
        aabb_3d_t face_aabb = calculate_3d_aabb(v_a, v_b, v_c);
        /* compute the range of chunks that span across the aabb - these are the chunks that overlap with the face */
        int chunk_xcol_min = (int)((face_aabb.x_min - world_x_min) / chunk_dx);
        int chunk_xcol_max = (int)((face_aabb.x_max - world_x_min) / chunk_dx);
        int chunk_zrow_min = (int)((face_aabb.z_min - world_z_min) / chunk_dz);
        int chunk_zrow_max = (int)((face_aabb.z_max - world_z_min) / chunk_dz);
        
        /* clamp boundaries on both ends because a triangle on the boundary can give a chunk index out of range */
        chunk_xcol_min = MAX(0, MIN(NUM_CHUNK_DIVISIONS - 1, chunk_xcol_min));
        chunk_xcol_max = MAX(0, MIN(NUM_CHUNK_DIVISIONS - 1, chunk_xcol_max));
        chunk_zrow_min = MAX(0, MIN(NUM_CHUNK_DIVISIONS - 1, chunk_zrow_min));
        chunk_zrow_max = MAX(0, MIN(NUM_CHUNK_DIVISIONS - 1, chunk_zrow_max));
        
        /* now add reference to object and face to the chunks */
        for(int z_row = chunk_zrow_min; z_row <= chunk_zrow_max; z_row++){
          for(int x_col = chunk_xcol_min; x_col <= chunk_xcol_max; x_col++){
            /* check if chunk already has reference to this object - if not, then create object entry */
            int chunk_object_idx = chunk_has_object_entry(z_row, x_col, object_id);
            if(chunk_object_idx == -1){ /* object does not exist in reference table */
              /* create new object slot in chunk */
              chunk_object_faces_t chunk_object_faces = {
                .object_id    = object_id,
                .face_indices = NULL,
                .num_faces    = 0
              };
              /* insert object reference into chunk */
              array_push(chunks[z_row][x_col].chunk_object_faces, chunk_object_faces);
              /* increment number of objects in chunk */
              chunks[z_row][x_col].num_chunk_objects++;
              chunk_object_idx = chunks[z_row][x_col].num_chunk_objects - 1;
            }

            /* insert face index into chunk's object reference */
            array_push(chunks[z_row][x_col].chunk_object_faces[chunk_object_idx].face_indices, face_i);
            /* increment number of faces for that object in chunk */
            chunks[z_row][x_col].chunk_object_faces[chunk_object_idx].num_faces++;

            /* update chunk's y-bounds with new face aabb's max/min height
               but the very first face assigned to the chunk needs to be special and set the initial size, otherwise for example a face totally above 0.0 would not update the initial value
            */
            if(chunks[z_row][x_col].empty){
              chunks[z_row][x_col].y_min = face_aabb.y_min;
              chunks[z_row][x_col].y_max = face_aabb.y_max;
              chunks[z_row][x_col].empty = false;
            }
            else{
              if(face_aabb.y_min < chunks[z_row][x_col].y_min) chunks[z_row][x_col].y_min = face_aabb.y_min;
              if(face_aabb.y_max > chunks[z_row][x_col].y_max) chunks[z_row][x_col].y_max = face_aabb.y_max;
            }
          }
        }
      }
    }
  }
}

/* search for object_id inside a chunk's object table, and return object's position in the table if it exists
   if object doesn't exist, return -1 */
int chunk_has_object_entry(int z_row, int x_col, uint32_t object_id){
  for(uint32_t chunk_object_faces_idx = 0; 
      chunk_object_faces_idx < chunks[z_row][x_col].num_chunk_objects; 
      chunk_object_faces_idx++
  )
    if(chunks[z_row][x_col].chunk_object_faces[chunk_object_faces_idx].object_id == object_id)
      return chunk_object_faces_idx;
    
  return -1;
}

/*
  loop through all collision_aabbs, and for each of the aabbs:
    test overlap with each chunk and add aabb to chunk
    NOTE: compute the chunk that the aabb possibly overlaps with by using:
      cx_min = (aabb_x_min - world_x_min) / chunk_size;
      cx_max = (aabb_x_max - world_x_min) / chunk_size;
      cz_min = ...
      cz_max = ...
      this gives a smaller range of chunks we loop through to test against, instead of looping thru all chunks.
  if there is overlap:  
    add collision_aabb_id, to chunk
*/

void populate_chunks_with_collision_aabbs(void){
  for(uint32_t collision_aabb_id = 0; collision_aabb_id < num_collision_aabbs; collision_aabb_id++){
    collision_aabb_t *collision_aabb = &collision_aabbs[collision_aabb_id];
    
    // compute the range of chunks that span across the collision aabb - these are the chunks that overlap with the collision aabb
    int chunk_xcol_min = (int)((collision_aabb->aabb.x_min - world_x_min) / chunk_dx);
    int chunk_xcol_max = (int)((collision_aabb->aabb.x_max - world_x_min) / chunk_dx);
    int chunk_zrow_min = (int)((collision_aabb->aabb.z_min - world_z_min) / chunk_dz);
    int chunk_zrow_max = (int)((collision_aabb->aabb.z_max - world_z_min) / chunk_dz);
    
    // clamp boundaries on both ends because an aabb on the boundary can give a chunk index out of range
    chunk_xcol_min = MAX(0, MIN(NUM_CHUNK_DIVISIONS - 1, chunk_xcol_min));
    chunk_xcol_max = MAX(0, MIN(NUM_CHUNK_DIVISIONS - 1, chunk_xcol_max));
    chunk_zrow_min = MAX(0, MIN(NUM_CHUNK_DIVISIONS - 1, chunk_zrow_min));
    chunk_zrow_max = MAX(0, MIN(NUM_CHUNK_DIVISIONS - 1, chunk_zrow_max));
    
    // now add reference to the collision aabb to the chunks
    for(int z_row = chunk_zrow_min; z_row <= chunk_zrow_max; z_row++){
      for(int x_col = chunk_xcol_min; x_col <= chunk_xcol_max; x_col++){
        // insert new collision_aabb id in chunk
        array_push(chunks[z_row][x_col].collision_aabbs, collision_aabb_id);
        // increment number of collision aabbs in chunk
        chunks[z_row][x_col].num_collision_aabbs++;
      }
    }
  }
}