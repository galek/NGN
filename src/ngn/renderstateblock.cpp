#include "renderstateblock.hpp"
#include "log.hpp"

namespace ngn {
    // These values represent the OpenGL default values
    bool RenderStateBlock::currentDepthWrite = true;
    DepthFunc RenderStateBlock::currentDepthFunc = DepthFunc::DISABLED;
    FaceDirections RenderStateBlock::currentCullFaces = FaceDirections::NONE;
    FaceOrientation RenderStateBlock::currentFrontFace = FaceOrientation::CCW;
    bool RenderStateBlock::currentBlendEnabled = false;
    RenderStateBlock::BlendFactor RenderStateBlock::currentBlendSrcFactor = RenderStateBlock::BlendFactor::ONE;
    RenderStateBlock::BlendFactor RenderStateBlock::currentBlendDstFactor = RenderStateBlock::BlendFactor::ZERO;
    RenderStateBlock::BlendEq RenderStateBlock::currentBlendEquation = RenderStateBlock::BlendEq::ADD;

    void RenderStateBlock::apply(bool force) const {
        if(mDepthWrite != currentDepthWrite || force) {
            glDepthMask(mDepthWrite ? GL_TRUE : GL_FALSE);
            currentDepthWrite = mDepthWrite;
        }

        if(mDepthFunc != currentDepthFunc || force) {
            if(mDepthFunc == DepthFunc::DISABLED) {
                glDisable(GL_DEPTH_TEST);
            } else {
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(static_cast<GLenum>(mDepthFunc));
            }
            currentDepthFunc = mDepthFunc;
        }

        if(mCullFaces != currentCullFaces || force) {
            if(mCullFaces == FaceDirections::NONE) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
                glCullFace(static_cast<GLenum>(mCullFaces));
            }
            currentCullFaces = mCullFaces;
        }

        if(mFrontFace != currentFrontFace || force) {
            glFrontFace(static_cast<GLenum>(mFrontFace));
            currentFrontFace = mFrontFace;
        }

        if(mBlendEnabled != currentBlendEnabled || force) {
            if(mBlendEnabled) {
                glEnable(GL_BLEND);
            } else {
                glDisable(GL_BLEND);
            }
            currentBlendEnabled = mBlendEnabled;
        }

        if(mBlendEnabled) {
            if(mBlendSrcFactor != currentBlendSrcFactor || mBlendDstFactor != currentBlendDstFactor || force) {
                glBlendFunc(static_cast<GLenum>(mBlendSrcFactor),
                            static_cast<GLenum>(mBlendDstFactor));
                currentBlendSrcFactor = mBlendSrcFactor;
                currentBlendDstFactor = mBlendDstFactor;
            }
            if(mBlendEquation != currentBlendEquation || force) {
                glBlendEquation(static_cast<GLenum>(mBlendEquation));
                currentBlendEquation = mBlendEquation;
            }
        }
    }
}