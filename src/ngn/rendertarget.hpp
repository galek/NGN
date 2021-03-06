#pragma once

#include <vector>
#include <utility>

#include <glad/glad.h>

#include "log.hpp"
#include "resource.hpp"
#include "texture.hpp"
#include "window.hpp"

namespace ngn {
    class Rendertarget {
    public:
        enum class Attachment : GLenum {
            COLOR0 = GL_COLOR_ATTACHMENT0,
            COLOR1 = GL_COLOR_ATTACHMENT1,
            COLOR2 = GL_COLOR_ATTACHMENT2,
            COLOR3 = GL_COLOR_ATTACHMENT3,
            COLOR4 = GL_COLOR_ATTACHMENT4,
            COLOR5 = GL_COLOR_ATTACHMENT5,
            COLOR6 = GL_COLOR_ATTACHMENT6,
            COLOR7 = GL_COLOR_ATTACHMENT7,
            COLOR8 = GL_COLOR_ATTACHMENT8,
            DEPTH = GL_DEPTH_ATTACHMENT,
            STENCIL = GL_STENCIL_ATTACHMENT,
            DEPTH_STENCIL = GL_DEPTH_STENCIL_ATTACHMENT
        };

    private:
        struct RenderbufferData {
            Attachment attachment;
            PixelFormat format;
            int width, height;
            GLuint rbo;

            RenderbufferData(Attachment _attachment, PixelFormat _format, int w = -1, int h = -1) :
                    attachment(_attachment), format(_format), width(w), height(h), rbo(0) {}
            ~RenderbufferData() {if(rbo != 0) glDeleteRenderbuffers(1, &rbo);}
        };

        GLuint mFBO;
        std::vector<std::pair<Attachment, Texture*> > mTextureAttachments;
        std::vector<RenderbufferData> mRenderbufferAttachments;
        int mWidth, mHeight;

        void prepare();

        static GLenum getTarget(bool read, bool write) {
            if(!read && !write) return 0;
            if(read && write) return GL_FRAMEBUFFER;
            if(read) return GL_READ_FRAMEBUFFER;
            if(write) return GL_DRAW_FRAMEBUFFER;
            return 0;
        }
    public:
        static Rendertarget* currentRendertargetDraw;
        static Rendertarget* currentRendertargetRead;

        static void unbind(bool read = true, bool write = true) {
            GLenum target = getTarget(read, write);
            if(target) {
                glBindFramebuffer(target, 0);
                if(read) currentRendertargetRead = nullptr;
                if(write) currentRendertargetDraw = nullptr;
                if(write) glViewport(0, 0, Window::currentWindow->getSize().x, Window::currentWindow->getSize().y);
            }
        }

        Rendertarget() : mFBO(0), mWidth(0xFFFFFF), mHeight(0xFFFFFF) {}
        Rendertarget(Texture& tex, PixelFormat depthRenderbufferFormat = PixelFormat::NONE) : Rendertarget() {
            attachTexture(Attachment::COLOR0, tex);
            if(depthRenderbufferFormat != PixelFormat::NONE) {
                attachRenderbuffer(Attachment::DEPTH, depthRenderbufferFormat, mWidth, mHeight);
            }
        }
        ~Rendertarget() {if(mFBO != 0) glDeleteFramebuffers(1, &mFBO);}

        void attachTexture(Attachment attachment, Texture& tex) {
            mTextureAttachments.push_back(std::make_pair(attachment, &tex));
            int w = tex.getWidth();
            int h = tex.getHeight();
            mWidth = mWidth < w ? mWidth : w;
            mHeight = mHeight < h ? mHeight : h;
        }

        template<typename... Args>
        void attachRenderbuffer(Args&& ...args) {
            mRenderbufferAttachments.emplace_back(std::forward<Args>(args)...);
            RenderbufferData& rBuffer = mRenderbufferAttachments.back();
            int w = rBuffer.width;
            int h = rBuffer.height;
            if(w < 0 || h < 0) {
                rBuffer.width = mWidth;
                rBuffer.height = mHeight;
            } else {
                mWidth = mWidth < w ? mWidth : w;
                mHeight = mHeight < h ? mHeight : h;
            }
        }

        const Texture* getTextureAttachment(Attachment attachment) const;

        inline void bind(bool read = true, bool write = true) {
            GLenum target = getTarget(read, write);
            if(target) {
                if(mFBO == 0) prepare();
                glBindFramebuffer(target, mFBO);
                glViewport(0, 0, mWidth, mHeight);
                if(read) currentRendertargetRead = this;
                if(write) currentRendertargetDraw = this;
            }
        }
    };

    struct Rendertexture : public Texture {
        Rendertarget renderTarget;

        Rendertexture(PixelFormat format, int width, int height = -1) {
            if(height < 0) height = width;
            setStorage(format, width, height);
            renderTarget.attachTexture(Rendertarget::Attachment::COLOR0, *this);
        }

        void renderTo() {renderTarget.bind();}
    };
}