#include <includes.h>

#include <iostream>
#include <ctime>
#include <cstring>
#include <fstream>
#include <pthread.h>

#include <RetiRenderer.h>
#include <RetiShader.h>
#include <RetiMesh.h>
#include <RetiTexture.h>
#include <RetiCamera.h>
#include <Core/RetiLog.h>

using namespace std;

string RetiRenderer::retiRoot;
int RetiRenderer::nInstances;
std::chrono::duration<long, std::milli> RetiRenderer::pause(1);
bool RetiRenderer::is_glfw_init = false;

void err_callback(int err, const char* desc)
{
    RetiLog::logln("GLFW ERROR: " + string(desc));
}

RetiRenderer::RetiRenderer()
{
    renderer_state = RETI_RENDERER_STATE_INACTIVE;
    win_x = 800;
    win_y = 600;
    title = "Reticle";
    clear_color_r = 0.8f;
    clear_color_g = 0.2f;
    clear_color_b = 0.05f;
    detach_renderer = true;
    constr_init_flags();

    common_shader = new RetiShader();
    cam = new RetiCamera(45, (win_x * 1.0) / win_y, 0.1, 1000.0);
    is_cam_user = false;

    cam->getTransform().translateTransform(0.0, 0.0, -1.0);

    nInstances++;
}

RetiRenderer::RetiRenderer(bool detach)
{
    renderer_state = RETI_RENDERER_STATE_INACTIVE;
    win_x = 800;
    win_y = 600;
    title = "Reticle";
    clear_color_r = 0.8f;
    clear_color_g = 0.2f;
    clear_color_b = 0.05f;
    detach_renderer = detach;
    constr_init_flags();

    common_shader = new RetiShader();
    cam = new RetiCamera(45, (win_x * 1.0) / win_y, 0.1, 1000.0);
    is_cam_user = false;

    cam->getTransform().translateTransform(0.0, 0.0, -1.0);

    nInstances++;
}

RetiRenderer::RetiRenderer(const RetiRenderer& other)
{
    renderer_state = RETI_RENDERER_STATE_INACTIVE;

    if(other.renderer_state != RETI_RENDERER_STATE_INACTIVE)
        RetiLog::logln("WARNING: Copy constructing from non-inactive renderer can cause undefined behavior.");

    win_x = other.win_x;
    win_y = other.win_y;
    title = other.title;
    clear_color_r = other.clear_color_r;
    clear_color_g = other.clear_color_g;
    clear_color_b = other.clear_color_b;
    detach_renderer = other.detach_renderer;
    constr_init_flags();

    common_shader = new RetiShader(*(other.common_shader));
    cam = new RetiCamera(*(other.cam));
    is_cam_user = false;

    cam->getTransform().translateTransform(0.0, 0.0, -1.0);

    meshes.reserve(other.meshes.size());

    for(int i = 0; i < other.meshes.size(); i++)
        meshes.push_back(other.meshes[i]);

    nInstances++;
}

void RetiRenderer::constr_init_flags()
{
    is_renderer_paused = false;
    do_breakout = false;
    is_glew_init = false;
    is_window_init = false;
}

RetiRenderer::~RetiRenderer()
{
    delete common_shader;

    if(!is_cam_user)
        delete cam;

    /// NOPE. All meshes, or scene objects, for that matter, are Reticle user's responsibility
    /*
    for(int i = 0; i < meshes.size(); i++)
        delete meshes[i];
    */

    meshes.clear();

    if(nInstances <= 1)
    {
        glfwTerminate();
        is_glfw_init = false;
    }

    nInstances--;
}

RetiRenderer& RetiRenderer::operator=(const RetiRenderer& other)
{
    RetiRenderer* ret = new RetiRenderer(other);
    return *ret;
}

const char* RetiRenderer::getFullVersionString()
{
    return version;
}

RetiRendererState RetiRenderer::getRedererState() const
{
    RetiRendererState cpy(renderer_state);
    return cpy;
}

void RetiRenderer::setDetachRenderer(bool detach)
{
    if(renderer_state == RETI_RENDERER_STATE_INACTIVE)
        detach_renderer = detach;
    else
        RetiLog::logln("WARNING: Cannot set detach renderer on non inactive Renderer, call ignored.");
}

void RetiRenderer::setReticleRootDirectory(const string& path)
{
    retiRoot = path;
}

string RetiRenderer::getReticleRootDirectory()
{
    return retiRoot;
}

void RetiRenderer::init()
{
    RetiLog::initLog();

    RetiTexture::initStatic();
}

void RetiRenderer::setWindowSize(int x, int y)
{
    if(renderer_state != RETI_RENDERER_STATE_INACTIVE)
    {
        RetiLog::logln("WARNING: Cannot set window size on non-inactive renderer, call ignored.");
        return;
    }
    RetiRenderer::win_x = x;
    RetiRenderer::win_y = y;
}

void RetiRenderer::setWindowTitle(string str)
{
    if(renderer_state != RETI_RENDERER_STATE_INACTIVE)
    {
        RetiLog::logln("WARNING: Cannot set window title on non-inactive renderer, call ignored.");
        return;
    }
    title = str;
}

void RetiRenderer::setClearColor(GLfloat R, GLfloat G, GLfloat B)
{
    if(renderer_state != RETI_RENDERER_STATE_INACTIVE)
    {
        RetiLog::logln("WARNING: Cannot set clear color on non-inactive renderer, call ignored.");
        return;
    }
    clear_color_r = R;
    clear_color_g = G;
    clear_color_b = B;
}

void RetiRenderer::init_glfw()
{
    if(is_glfw_init)
        return;
    RetiLog::logln("Initializing GLFW.");
    glfwInit();
    glfwSetErrorCallback(err_callback);


    is_glfw_init = true;

    RetiLog::logln("GLFW initialization complete.");
}

void RetiRenderer::create_window()
{
    if(is_window_init)
        return;
    RetiLog::logln("Creating window");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, 0);
    window = glfwCreateWindow(win_x, win_y, title.c_str(), nullptr, nullptr);
    if(window==nullptr)
        RetiLog::logln("FAILED TO CREATE WINDOW " + to_string(win_x) + " " + to_string(win_y));
    glfwMakeContextCurrent(this->window);
    glfwSwapInterval(0);
    is_window_init = true;
    RetiLog::logln("Created window");
}

void RetiRenderer::init_glew()
{
    if(is_glew_init)
        return;
    RetiLog::logln("Initializing GLEW.");
    glewExperimental = GL_TRUE;
    int res = glewInit();
    if(res != GLEW_OK)
    {
        RetiLog::logln(string((const char*) glewGetErrorString(res)));
    }

    int w, h;
    glfwGetFramebufferSize(this->window, &w, &h);
    glViewport(0, 0, w, h);

    is_glew_init = true;

    RetiLog::logln("GLEW initialization complete.");
}

void RetiRenderer::init_renderer()
{
    this->init_glfw();
    this->create_window();
    this->init_glew();
    this->init_shader();
    this->init_meshes();

    glClearColor(clear_color_r, clear_color_g, clear_color_b, 1.0f);
}

void RetiRenderer::init_shader()
{
    common_shader->loadShader();
}

void RetiRenderer::unload_shader()
{
    common_shader->unloadShader();
}

void RetiRenderer::init_meshes()
{
    for(int i = 0; i < meshes.size(); i++)
        meshes[i]->loadMesh();
}

void RetiRenderer::unload_meshes()
{
    for(int i = 0; i < meshes.size(); i++)
        meshes[i]->unloadMesh();
}

void RetiRenderer::render_internal()
{
    renderer_state = RETI_RENDERER_STATE_LOADING;
    this->init_renderer();

    renderer_state = RETI_RENDERER_STATE_ACTIVE;
    while(!glfwWindowShouldClose(this->window))
    {
        if(is_renderer_paused)
        {
            this_thread::sleep_for(pause);
            continue;
        }

        if(do_breakout)
        {
            RetiLog::logln("Breaking Out.");
            do_breakout = false;
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);

        common_shader->setActiveShader();
        for(int i = 0; i < meshes.size(); i++)
            meshes[i]->render(cam);

        glfwSwapBuffers(RetiRenderer::window);
    }
    gput_post_renderer_cleanup();
}

void RetiRenderer::gput_post_renderer_cleanup()
{
    unload_meshes();
    unload_shader();
    //delete window;
    glfwDestroyWindow(window);
}

void RetiRenderer::addMesh(RetiMesh* mesh)
{
    meshes.push_back(mesh);
}

void RetiRenderer::addMesh(const float* vertCoords, const float* texCoords, const int n_verts,
                           const unsigned int* tris, const int n_tris)
{
    RetiMesh* mesh = new RetiMesh(vertCoords, texCoords, n_verts, tris, n_tris);
    meshes.push_back(mesh);
}

void RetiRenderer::startRenderer()
{
    if(detach_renderer)
        gpu_thread = new thread(&RetiRenderer::render_internal, this);
    else
        render_internal();
}

void RetiRenderer::stopRenderer()
{
    RetiLog::logln("Stopping Renderer");
    do_breakout = true;
    gpu_thread->join();
    delete gpu_thread;
    is_window_init = false;
    is_glew_init = false;
    RetiLog::logln("Renderer stopped");
}

void RetiRenderer::pauseRenderer()
{
    is_renderer_paused = true;
}

void RetiRenderer::resumeRenderer()
{
    is_renderer_paused = false;
}

void RetiRenderer::useCamera(RetiCamera* n_cam)
{
    if(!is_cam_user)
        delete cam;
    cam = n_cam;
    is_cam_user = true;
}
