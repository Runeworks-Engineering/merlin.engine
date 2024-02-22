#pragma once
#include "Merlin/Core/Core.h"

namespace Merlin {
    class RenderBuffer
    {
    public:
        RenderBuffer(int samples = 0);
        ~RenderBuffer();

        void Bind();
        void Unbind();
        void Resize(GLsizei width, GLsizei height);
        
        void AllocateStorage(GLsizei width, GLsizei height, GLenum internalFormat);

        inline GLuint id() const { return _RenderbufferID; }


    private:
        GLuint _RenderbufferID;
        GLuint _samples;
        GLenum _format;

    };
    typedef RenderBuffer RBO;
}

