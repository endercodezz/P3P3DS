#ifndef _LENNY_H
#define _LENNY_H

typedef struct {
    float x, y, z;
    float nx, ny, nz;
} Vertex;

extern const Vertex g_VertexList[3345];
#define VERTEX_LIST_COUNT (sizeof(g_VertexList)/sizeof(g_VertexList[0]))

#endif // _LENNY_H