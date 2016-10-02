#include "material.hpp"
#include "renderer.hpp"

namespace ngn {
    bool Material::staticInitialized = false;
    Material* Material::fallback = nullptr;

    void Material::staticInitialize() {
        Material::staticInitialized = true;

        delete new Shader; // force static initialization of Shader to make sure fallbacks are initialized :/
        Material::fallback = new Material;
        // Our unset ResourceHandles will automatically fall back to the shader fallbacks
        Material::fallback->addPass(Renderer::AMBIENT_PASS);
    }

    ShaderProgram* Material::getShaderPermutation(uint64_t permutationHash, const FragmentShader* frag, const VertexShader* vert,
            const std::string& fragDefines, const std::string& vertDefines) {
        using keyType = std::tuple<uint64_t, const FragmentShader*, const VertexShader*>;
        static std::unordered_map<keyType, ShaderProgram*, hash_tuple::hash<keyType> > shaderCache;

        auto keyTuple = std::make_tuple(permutationHash, frag, vert);
        auto it = shaderCache.find(keyTuple);
        if(it == shaderCache.end()) {
            ShaderProgram* prog = new ShaderProgram;
            if(!prog->compileAndLinkFromStrings(frag->getFullString(fragDefines).c_str(),
                                                vert->getFullString(vertDefines).c_str())) {
                delete prog;
                return nullptr;
            }
            shaderCache.insert(std::make_pair(keyTuple, prog));
            return prog;
        } else {
            return it->second;
        }
    }

    void Material::validate() const {
        if((mBlendMode == BlendMode::MODULATE || mBlendMode == BlendMode::SCREEN) && (hasPass(Renderer::LIGHT_PASS)))
            LOG_WARNING("Blend mode MODULATE and SCREEN don't work properly with lit materials!");
    }

    void Material::setBlendMode(BlendMode mode) {
        mBlendMode = mode;
        switch(mode) {
            case BlendMode::REPLACE:
                mStateBlock.setBlendEnabled(false);
                mStateBlock.setDepthWrite(true);
                break;
            case BlendMode::TRANSLUCENT:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::SRC_ALPHA, RenderStateBlock::BlendFactor::ONE_MINUS_SRC_ALPHA);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::ADD:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::ONE, RenderStateBlock::BlendFactor::ONE);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::MODULATE:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::DST_COLOR, RenderStateBlock::BlendFactor::ZERO);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::SCREEN:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::ONE, RenderStateBlock::BlendFactor::ONE_MINUS_SRC_COLOR);
                mStateBlock.setDepthWrite(false);
                break;
        }
        validate();
    }

    bool Material::load(const char* filename) {
        //*this = Material(); // re-initialize
        return false;
    }
}