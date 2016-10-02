#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ngn {
    // Maybe introduce a new class Shader that represents a Shader object, so they can be
    // attached to multiple program objects
    class ShaderProgram {
    public:
        enum class ShaderType {
            VERTEX = 0, FRAGMENT = 1
        };
        static const char* shaderTypeNames[];
        enum class Status {
            EMPTY, UNLINKED, LINKED, COMPILE_SHADER_FAILED, LINK_FAILED
        };

        using ShaderVariableLocation = GLint;

        static const ShaderProgram* currentShaderProgram;

    private:
        GLuint mProgramObject;
        std::vector<GLuint> mShaderObjects;
        Status mStatus;
        mutable std::unordered_map<std::string, ShaderVariableLocation> mAttributeLocations; // only caches
        mutable std::unordered_map<std::string, ShaderVariableLocation> mUniformLocations;

    public:
        ShaderProgram();
        ShaderProgram(const char* fragfile, const char* vertfile);
        ~ShaderProgram();

        bool compileShaderFromString(const char* source, ShaderType type);
        bool compileVertShaderFromString(const char* source) {
            return compileShaderFromString(source, ShaderType::VERTEX);
        }
        bool compileFragShaderFromString(const char* source) {
            return compileShaderFromString(source, ShaderType::FRAGMENT);
        }
        bool compileAndLinkFromStrings(const char* frag, const char* vert) {
            return  compileFragShaderFromString(frag) &&
                    compileVertShaderFromString(vert) &&
                    link();
        }

        // Should this even exist?
        bool compileShaderFromFile(const char* filename, ShaderType type);
        bool compileVertShaderFromFile(const char* filename) {
            return compileShaderFromFile(filename, ShaderType::VERTEX);
        }
        bool compileFragShaderFromFile(const char* filename) {
            return compileShaderFromFile(filename, ShaderType::FRAGMENT);
        }
        bool compileAndLinkFromFiles(const char* fragfile, const char* vertfile) {
            return  compileFragShaderFromFile(fragfile) &&
                    compileVertShaderFromFile(vertfile) &&
                    link();
        }

        ShaderVariableLocation getAttributeLocation(const std::string& name) const;
        ShaderVariableLocation getUniformLocation(const std::string& name) const;

        bool link();

        inline void bind() const {
            if(currentShaderProgram != this) {
                glUseProgram(mProgramObject);
                currentShaderProgram = this;
            }
        }
        // This could be static, but I want it to look like other bindables
        void unbind() const {
            glUseProgram(0);
            currentShaderProgram = nullptr;
        }

        GLuint getProgramObject() const {
            return mProgramObject;
        }
        Status getStatus() const {
            return mStatus;
        }

        template<typename T>
        void setUniform(const std::string& name, const T& val) const {
            ShaderVariableLocation loc = getUniformLocation(name);
            if(loc != -1) setUniform(loc, val);
        }

        void setUniform(ShaderVariableLocation loc, int value) const {
            glUniform1i(loc, value);
        }

        void setUniform(ShaderVariableLocation loc, float value) const {
            glUniform1f(loc, value);
        }

        void setUniform(ShaderVariableLocation loc, const glm::vec2& val) const {
            glUniform2fv(loc, 1, glm::value_ptr(val));
        }

        void setUniform(ShaderVariableLocation loc, const glm::vec3& val) const {
            glUniform3fv(loc, 1, glm::value_ptr(val));
        }

        void setUniform(ShaderVariableLocation loc, const glm::vec4& val) const {
            glUniform4fv(loc, 1, glm::value_ptr(val));
        }

        void setUniform(ShaderVariableLocation loc, const glm::mat2& val) const {
            glUniformMatrix2fv(loc, 1, GL_FALSE, glm::value_ptr(val));
        }

        void setUniform(ShaderVariableLocation loc, const glm::mat3& val) const {
            glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(val));
        }

        void setUniform(ShaderVariableLocation loc, const glm::mat4& val) const {
            glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(val));
        }
    };
}