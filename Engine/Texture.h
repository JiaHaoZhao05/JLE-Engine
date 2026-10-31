#pragma once

#include <string>
#include <glad/glad.h>

class Texture {
public:
    Texture(const std::string& fileName);
    ~Texture();

    void Bind(GLenum textureUnit) const;

    //Texture(const Texture&) = delete;
    //Texture& operator=(const Texture&) = delete;

    std::string m_fileName;
    GLuint m_textureObj = 0;
    size_t byteSize;
};