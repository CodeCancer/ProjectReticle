#ifndef RETIMESH_H_INCLUDED
#define RETIMESH_H_INCLUDED

#include <includes.h>

#include <vector>

#include <RetiMaterial.h>
#include <Core/RetiSpinlock.h>
#include <Math/RetiTransform.h>

class RetiCamera;

class RetiMesh
{
private:

    RetiTransform transf;
    RetiSpinlock transf_lock;

    // Internal OpenGL handles
    GLfloat* vertex_data;               //Treat as const after constructor call
    const int vertex_data_size;
    GLuint* triangle;                   //Treat as const after constructor call
    const int triangle_size;

    RetiMaterial* material;

    bool is_loaded;

    GLuint vertex_buffer_id;
    GLuint triangle_buffer_id;
    GLuint vao_id;

public:

    RetiMesh(const float* vert_coords, const float* tex_coords, const int n_verts,
             const unsigned int* tris, const int n_tris, RetiMaterial* mat = new RetiMaterial());
    RetiMesh(const float* vertData, const int vSize, const unsigned int* tris, const int tSize, RetiMaterial* mat = new RetiMaterial());
    RetiMesh(const RetiMesh& other);
    ~RetiMesh();
    RetiMesh& operator=(const RetiMesh &) = delete;

    // Initialize OpenGL
    void loadMesh();
    void unloadMesh();

    RetiTransform& getTransform();

    //Render with OpenGL
    void render(RetiCamera* cam);
};

#endif // RETIMESH_H_INCLUDED
