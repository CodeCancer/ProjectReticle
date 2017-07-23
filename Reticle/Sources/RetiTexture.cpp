#include <includes.h>

#include <stb_image.h>

#include <includes.h>

#include <RetiRenderer.h>
#include <RetiTexture.h>
#include <Core/RetiLog.h>

using namespace std;

bool RetiTexture::is_static_init = false;
RetiTexture* RetiTexture::defaultTexture;
unordered_map<string, RetiTextureData*> RetiTexture::path_to_data;

void RetiTexture::load_texture_from_file(string path)
{
    #ifdef VERBOSE_ON
    RetiLog::logln("Loading file: " + path);
    #endif // VERBOSE_ON

    if((texture_struct = path_to_data[path]) != nullptr)
    {
        (texture_struct->nUser)++;
        return;
    }

    texture_struct = new RetiTextureData();

    texture_struct->data = stbi_load(path.c_str(), &(texture_struct->sSize), &(texture_struct->tSize), &(texture_struct->nChannels), 0);
    if(texture_struct->data == nullptr)
        RetiLog::logln("WARNING : Unable to load texture data, data is nullptr, segfault imminent.");

    texture_struct->isLoaded = false;
    texture_struct->nUser = 1;
}

RetiTexture::RetiTexture()
{
    filter_mode = RETI_TEXFLTR_LINEAR;
    mipmap_mode = RETI_MIPFLTR_LINEAR;
    wrap_s = RETI_TEXWRAP_MIRROR;
    wrap_t = RETI_TEXWRAP_MIRROR;
    texture_unit = GL_TEXTURE0;
    load_texture_from_file(RetiRenderer::getReticleRootDirectory() + "/Resources/Textures/default.tga");
}

RetiTexture::RetiTexture(string path)
{
    filter_mode = RETI_TEXFLTR_LINEAR;
    mipmap_mode = RETI_MIPFLTR_NEAREST;
    wrap_s = RETI_TEXWRAP_MIRROR;
    wrap_t = RETI_TEXWRAP_MIRROR;
    texture_unit = GL_TEXTURE0;
    load_texture_from_file(path);
}

RetiTexture::RetiTexture(string path, RetiTextureFilterMode fmode, RetiMipmapFilterMode mmode,
                         RetiTextureWrapMode s_w, RetiTextureWrapMode t_w, int bind_location)
{
    filter_mode = fmode;
    mipmap_mode = mmode;
    wrap_s = s_w;
    wrap_t = t_w;
    texture_unit = GL_TEXTURE0 + bind_location;
    if(bind_location > 15)
        RetiLog::logln("WARNING: Support for bind locations above 15 is platform dependent.");
    load_texture_from_file(path);
}

RetiTexture::RetiTexture(const RetiTexture& other)
{
    filter_mode = other.filter_mode;
    mipmap_mode = other.mipmap_mode;
    wrap_s = other.wrap_s;
    wrap_t = other.wrap_t;
    if(texture_unit > 15)
        RetiLog::logln("WARNING: Support for bind locations above 15 is platform dependent.");
    texture_unit = other.texture_unit;

    texture_struct = other.texture_struct;
    (texture_struct->nUser)++;
}

RetiTexture& RetiTexture::operator=(const RetiTexture& other)
{
    RetiTexture* ret = new RetiTexture(other);
    return *ret;
}

RetiTexture::~RetiTexture()
{
    ++(texture_struct->nUser);
    if(texture_struct->nUser > 0)
        return;
    if(texture_struct->isLoaded)
    {
        RetiLog::logln("WARNING: Texture loaded even when last instance is being deleted, attempting unload.");
        unloadTexture();
    }
    stbi_image_free(texture_struct->data);
}

void RetiTexture::setMipmapFilterMode(RetiMipmapFilterMode mode)
{
    mipmap_mode = mode;
    translate_params_to_gpu();
}

void RetiTexture::setTextureFilterMode(RetiTextureFilterMode mode)
{
    filter_mode = mode;
    translate_params_to_gpu();
}

void RetiTexture::setWrapModes(RetiTextureWrapMode w_s, RetiTextureWrapMode w_t)
{
    wrap_s = w_s;
    wrap_t = w_t;
    translate_params_to_gpu();
}

void RetiTexture::setBindLocation(int bnd)
{
    if(bnd > 15)
        RetiLog::logln("WARNING: Support for bind locations above 15 is platform dependent.");
    texture_unit = GL_TEXTURE0 + bnd;
}

RetiTextureFilterMode RetiTexture::getTextureFilterMode()
{
    return filter_mode;
}

RetiMipmapFilterMode RetiTexture::getMipmapFilterMode()
{
    return mipmap_mode;
}

RetiTextureWrapMode RetiTexture::getTextureWrapModeS()
{
    return wrap_s;
}

RetiTextureWrapMode RetiTexture::getTextureWrapModeT()
{
    return wrap_t;
}

int RetiTexture::getBindLocation()
{
    return texture_unit - GL_TEXTURE0;
}

int RetiTexture::getSizeS()
{
    return texture_struct->sSize;
}

int RetiTexture::getSizeT()
{
    return texture_struct->tSize;
}

void RetiTexture::loadTexture()
{
    ++(texture_struct->nLoaded);
    if(texture_struct->isLoaded)
        return;

    translate_params_to_gpu();

    glGenTextures(1, &(texture_struct->gpuTextureId));
    glBindTexture(GL_TEXTURE_2D, (texture_struct->gpuTextureId));

    // Load and gen mipmaps
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_struct->sSize, texture_struct->tSize, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_struct->data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    texture_struct->isLoaded = true;
}

void RetiTexture::translate_params_to_gpu()
{
    //WRAP MODES
    switch(wrap_s)
    {
        case RETI_TEXWRAP_CLAMP:    gpu_wrap_s = GL_CLAMP_TO_EDGE; break;
        case RETI_TEXWRAP_MIRROR:   gpu_wrap_s = GL_MIRRORED_REPEAT; break;
        case RETI_TEXWRAP_REPEAT:   gpu_wrap_s = GL_REPEAT; break;
    }
    switch(wrap_t)
    {
        case RETI_TEXWRAP_CLAMP:    gpu_wrap_t = GL_CLAMP_TO_EDGE; break;
        case RETI_TEXWRAP_MIRROR:   gpu_wrap_t = GL_MIRRORED_REPEAT; break;
        case RETI_TEXWRAP_REPEAT:   gpu_wrap_t = GL_REPEAT; break;
    }

    // Magnification Filters
    switch(filter_mode)
    {
        case RETI_TEXFLTR_LINEAR:   gpu_mag_filter = GL_LINEAR; break;
        case RETI_TEXFLTR_NEAREST:  gpu_mag_filter = GL_NEAREST; break;
    }

    // Minimization filters
    if(filter_mode == RETI_TEXFLTR_LINEAR)
    {
        switch(mipmap_mode)
        {
            case RETI_MIPFLTR_LINEAR:   gpu_min_filter = GL_LINEAR_MIPMAP_LINEAR; break;
            case RETI_MIPFLTR_NEAREST:  gpu_min_filter = GL_LINEAR_MIPMAP_NEAREST; break;
        }
    }
    else if(filter_mode == RETI_TEXFLTR_NEAREST)
    {
        switch(mipmap_mode)
        {
            case RETI_MIPFLTR_LINEAR:   gpu_min_filter = GL_NEAREST_MIPMAP_LINEAR; break;
            case RETI_MIPFLTR_NEAREST:  gpu_min_filter = GL_NEAREST_MIPMAP_NEAREST; break;
        }
    }
}

void RetiTexture::unloadTexture()
{
    (texture_struct->nLoaded)--;
    if(!texture_struct->isLoaded)
        return;
    glBindTexture(GL_TEXTURE_2D, 0);
    if(texture_struct->nLoaded <= 0)
    {
        glDeleteTextures(1, &texture_struct->gpuTextureId);
        texture_struct->isLoaded = false;
    }
}

void RetiTexture::useTexture()
{
    if(!texture_struct->isLoaded)
        loadTexture();
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_2D, texture_struct->gpuTextureId);
    load_params();
}

void RetiTexture::useTextureAt(int bind_point)
{
    if(!texture_struct->isLoaded)
        loadTexture();
    glActiveTexture(GL_TEXTURE0 + bind_point);
    glBindTexture(GL_TEXTURE_2D, texture_struct->gpuTextureId);
    load_params();
}

void RetiTexture::load_params()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gpu_min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gpu_mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gpu_wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gpu_wrap_t);
}

RetiTexture* RetiTexture::getDefaultTexture()
{
    return defaultTexture;
}

void RetiTexture::initStatic()
{
    if(is_static_init)
        return;
    defaultTexture = new RetiTexture();
    is_static_init = true;
}
