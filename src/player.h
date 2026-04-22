#ifndef PLAYER_H
#define PLAYER_H

typedef struct{

} player_t;
/*
Collision query flow

Each frame or movement step:

compute player chunk
visit player chunk + 8 neighbors
for each collision AABB ID in those chunks:
skip if already stamped this step
Y overlap test
circle-vs-AABB in XZ
push player out if overlapping

That is the whole idea.
*/
#endif