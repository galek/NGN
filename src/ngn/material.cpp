#include "material.hpp"

namespace ngn {
    void Material::validate() const {
        if(mLit && (mBlendMode == BlendMode::MODULATE || mBlendMode == BlendMode::SCREEN))
            LOG_WARNING("Blend mode MODULATE and SCREEN don't work properly with lit materials!");
    }

    void Material::setBlendMode(BlendMode mode) {
        mBlendMode = mode;
        switch(mode) {
            case BlendMode::REPLACE:
                mStateBlock.setBlendEnabled(false);
                break;
            case BlendMode::TRANSLUCENT:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::SRC_ALPHA, RenderStateBlock::BlendFactor::ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::ADD:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::ONE, RenderStateBlock::BlendFactor::ONE);
                break;
            case BlendMode::MODULATE:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::DST_COLOR, RenderStateBlock::BlendFactor::ZERO);
                break;
            case BlendMode::SCREEN:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::ONE, RenderStateBlock::BlendFactor::ONE_MINUS_SRC_COLOR);
                break;
        }
        validate();
    }
}