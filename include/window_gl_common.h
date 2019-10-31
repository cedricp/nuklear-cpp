
inline GLuint
gl_create_texture(const NkTexture& texture)
{
	GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLenum pixel_type = GL_UNSIGNED_BYTE;
    GLenum format = GL_RGBA;
    GLenum internal_format = GL_RGBA;

    if (texture.pixel_data_type == NkTexture::TYPE_UCHAR){
    	pixel_type = GL_UNSIGNED_BYTE;
    } else if (texture.pixel_data_type == NkTexture::TYPE_FLOAT){
    	pixel_type = GL_FLOAT;
    }
#ifdef GLES2
    else if (texture.internal_format == NkTexture::RGB_BYTE){
    	internal_format = GL_RGB;
    } else if (texture.internal_format == NkTexture::RGBA_BYTE){
    	internal_format = GL_RGBA;
    } else if (texture.internal_format == NkTexture::RGB_FLOAT){
    	internal_format = GL_RGB32F_EXT;
    } else if (texture.internal_format == NkTexture::RGBA_FLOAT){
    	internal_format = GL_RGBA32F_EXT;
    } else if (texture.internal_format == NkTexture::RGB_HALF){
    	internal_format = GL_RGB16F_EXT;
    } else if (texture.internal_format == NkTexture::RGBA_HALF){
    	internal_format = GL_RGBA16F_EXT;
    }
#else
    else if (texture.internal_format == NkTexture::RGB_BYTE){
    	internal_format = GL_RGB;
    } else if (texture.internal_format == NkTexture::RGBA_BYTE){
    	internal_format = GL_RGBA;
    } else if (texture.internal_format == NkTexture::RGB_FLOAT){
    	internal_format = GL_RGB32F;
    } else if (texture.internal_format == NkTexture::RGBA_FLOAT){
    	internal_format = GL_RGBA32F;
    } else if (texture.internal_format == NkTexture::RGB_HALF){
    	internal_format = GL_RGB16F;
    } else if (texture.internal_format == NkTexture::RGBA_HALF){
    	internal_format = GL_RGBA16F;
    }
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture.width, texture.height, 0, format, pixel_type, texture.data);
    return id;
}
