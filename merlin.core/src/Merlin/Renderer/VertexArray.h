#pragma once

#include "VertexBuffer.h"
#include <vector>
#include <glad/glad.h>

namespace Merlin::Renderer {
    struct VertexBufferElement {
        unsigned int type;
        unsigned int count;
        unsigned char normalized;

        static unsigned int GetTypeSize(unsigned int type) {
            switch (type) {
            case GL_FLOAT:          return 4;
            case GL_UNSIGNED_INT:   return 4;
            case GL_UNSIGNED_BYTE:  return 1;
            }
            return 0;
        }
    };

    class VertexBufferLayout {
    public:
        VertexBufferLayout() : m_Stride(0) {}

        template<typename T>
        void Push(unsigned int count) {
            static_assert(false);
        }

        template<>
        void Push<float>(unsigned int count) {
            m_Elements.push_back({ GL_FLOAT, count, GL_FALSE });
            m_Stride += VertexBufferElement::GetTypeSize(GL_FLOAT) * count;
        }

        template<>
        void Push<unsigned int>(unsigned int count) {
            m_Elements.push_back({ GL_UNSIGNED_INT, count, GL_FALSE });
            m_Stride += VertexBufferElement::GetTypeSize(GL_UNSIGNED_INT) * count;
        }

        template<>
        void Push<unsigned char>(unsigned int count) {
            m_Elements.push_back({ GL_UNSIGNED_BYTE, count, GL_TRUE });
            m_Stride += VertexBufferElement::GetTypeSize(GL_BYTE) * count;
        }

        inline const std::vector<VertexBufferElement>
            GetElements() const { return m_Elements; }

        inline unsigned int
            GetStride() const { return m_Stride; }

    private:
        std::vector<VertexBufferElement> m_Elements;
        GLuint m_Stride;
    };

    class VertexArray {
    public:
        VertexArray();
        ~VertexArray();

        void Bind();
        void Unbind();

        void AddBuffer(VertexBuffer& vb, const VertexBufferLayout& layout);
    private:
        GLuint m_RendererID;
    };

    typedef VertexArray VAO;

}