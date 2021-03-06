#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.hpp"

namespace ngn {
    GLuint Mesh::lastBoundVAO = 0;

    void Mesh::compile() {
        if(mVAO == 0) glGenVertexArrays(1, &mVAO);
        glBindVertexArray(mVAO);

        // Not sure if this should be in VertexFormat
        for(auto& vData : mVertexBuffers) {
            vData->bind();
            const VertexFormat& format = vData->getVertexFormat();
            const std::vector<VertexAttribute>& attributes = format.getAttributes();
            for(size_t i = 0; i < attributes.size(); ++i) {
                const VertexAttribute& attr = attributes[i];
                int location = static_cast<int>(attr.type);
                glEnableVertexAttribArray(location);
                glVertexAttribPointer(location, attr.alignedNum, static_cast<GLenum>(attr.dataType),
                                      attr.normalized ? GL_TRUE : GL_FALSE,
                                      format.getStride(), reinterpret_cast<GLvoid*>(format.getAttributeOffset(i)));
                if(attr.divisor > 0) glVertexAttribDivisor(location, attr.divisor);
            }
        }

        // for ARRAY_BUFFER only the calls to glEnableVertexAttribArray/glEnableVertexPointer are stored
        // so unbind now.
        mVertexBuffers.back()->unbind();

        if(mIndexBuffer != nullptr) mIndexBuffer->bind();

        glBindVertexArray(0);

        // VAO stores the last bound ELEMENT_BUFFER state, so as soon as the VAO is unbound, unbind the VBO
        if(mIndexBuffer != nullptr) mIndexBuffer->unbind();
    }

    // Transform positions, normals, tangents and bitangents
    void Mesh::transform(const glm::mat4& transform, const std::vector<AttributeType>& pointAttributes,
                         const std::vector<AttributeType>& vectorAttributes) {
        for(auto attrType : pointAttributes) {
            if(!hasAttribute(attrType)) continue;
            auto attr = getAccessor<glm::vec3>(attrType);
            for(size_t i = 0; i < attr.getCount(); ++i) {
                attr[i] = glm::vec3(transform * glm::vec4(attr.get(i), 1.0f));
            }
        }

        for(auto attrType : vectorAttributes) {
            if(!hasAttribute(attrType)) continue;
            auto attr = getAccessor<glm::vec3>(attrType);
            for(size_t i = 0; i < attr.getCount(); ++i) {
                attr[i] = glm::vec3(transform * glm::vec4(attr.get(i), 0.0f));
            }
        }
    }

    void Mesh::normalize(bool rescale) {
        std::pair<glm::vec3, float> bSphere;
        glm::mat4 t = glm::translate(glm::mat4(), glm::vec3(-bSphere.first.x, -bSphere.first.y, -bSphere.first.z));
        if(rescale) t = glm::scale(t, glm::vec3(1.0f / bSphere.second));
        transform(t);
    }

    glm::vec3 Mesh::center() const {
        return boundingSphere().first;
    }

    std::pair<glm::vec3, float> Mesh::boundingSphere() const {
        AABoundingBox bBox = boundingBox();
        glm::vec3 center = (bBox.min + bBox.max) * 0.5f;
        float radius = glm::length(center - bBox.min);
        return std::make_pair(center, radius);
    }

    const AABoundingBox& Mesh::boundingBox() const {
        if(mBBoxDirty) {
            auto position = getAccessor<glm::vec3>(AttributeType::POSITION);
            mBoundingBox.min = mBoundingBox.max = position.get(0);
            for(size_t i = 1; i < position.getCount(); ++i) mBoundingBox.fitPoint(position.get(i));
            mBBoxDirty = false;
        }
        return mBoundingBox;
    }

    Mesh* assimpMesh(aiMesh* mesh, const VertexFormat& format) {
        auto ngnMesh = new Mesh(Mesh::DrawMode::TRIANGLES);
        ngnMesh->addVertexBuffer(format, mesh->mNumVertices);

        if(mesh->HasPositions() && format.hasAttribute(AttributeType::POSITION)) {
            auto position = ngnMesh->getAccessor<glm::vec3>(AttributeType::POSITION);
            for(size_t i = 0; i < mesh->mNumVertices; ++i) {
                aiVector3D& vec = mesh->mVertices[i];
                position[i] = glm::vec3(vec.x, vec.y, vec.z);
            }
        }

        if(mesh->HasNormals() && format.hasAttribute(AttributeType::NORMAL)) {
            auto normal = ngnMesh->getAccessor<glm::vec3>(AttributeType::NORMAL);
            for(size_t i = 0; i < mesh->mNumVertices; ++i) {
                aiVector3D& vec = mesh->mNormals[i];
                normal[i] = glm::vec3(vec.x, vec.y, vec.z);
            }
        }

        if(mesh->HasTextureCoords(0) && format.hasAttribute(AttributeType::TEXCOORD0)) {
            auto texCoord = ngnMesh->getAccessor<glm::vec2>(AttributeType::TEXCOORD0);
            for(size_t i = 0; i < mesh->mNumVertices; ++i) {
                aiVector3D& vec = mesh->mTextureCoords[0][i];
                texCoord[i] = glm::vec2(vec.x, vec.y);
            }
        }

        if(mesh->HasFaces()) {
            IndexBuffer* iData = ngnMesh->setIndexBuffer(getIndexBufferType(mesh->mNumVertices), mesh->mNumFaces*3);
            size_t index = 0;
            for(size_t i = 0; i < mesh->mNumFaces; ++i) {
                (*iData)[index++] = mesh->mFaces[i].mIndices[0];
                (*iData)[index++] = mesh->mFaces[i].mIndices[1];
                (*iData)[index++] = mesh->mFaces[i].mIndices[2];
            }
        }

        return ngnMesh;
    }

    std::vector<std::pair<std::string, Mesh*> > assimpMeshes(const char* filename, bool merge, const VertexFormat& format) {
        Assimp::Importer importer;
        /*importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
            aiComponent_NORMALS | aiComponent_TANGENTS_AND_BITANGENTS | aiComponent_COLORS |
            aiComponent_LIGHTS | aiComponent_CAMERAS);*/
        const aiScene *scene = importer.ReadFile(filename, aiProcessPreset_TargetRealtime_Fast | aiProcess_OptimizeMeshes | (merge ? aiProcess_OptimizeGraph : 0));
        if(!scene) {
            LOG_ERROR("Mesh file '%s' could not be loaded!\n", importer.GetErrorString());
        }

        std::vector<std::pair<std::string, Mesh*> > meshes;
        for(size_t i = 0; i < scene->mNumMeshes; ++i) {
            meshes.push_back(std::make_pair(std::string(scene->mMeshes[i]->mName.C_Str()), assimpMesh(scene->mMeshes[i], format)));
        }

        return meshes;
    }

    // width, height, depth along x, y, z, center is 0, 0, 0
    Mesh* boxMesh(float width, float height, float depth, const VertexFormat& format) {
        static float vertices[] = {
            // +z
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,

            // -z
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            // +x
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,

            // -x
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,

            // +y
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,

            // -y
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f
        };

        static uint8_t indices[6] = {
            0, 1, 3,
            3, 1, 2
        };

        static float normals[6][3] = {
            { 0.0f,  0.0f,  1.0f},
            { 0.0f,  0.0f, -1.0f},
            { 1.0f,  0.0f,  0.0f},
            {-1.0f,  0.0f,  0.0f},
            { 0.0f,  1.0f,  0.0f},
            { 0.0f, -1.0f,  0.0f},
        };

        glm::vec3 size = glm::vec3(width * 0.5f, height * 0.5f, depth * 0.5f);

        Mesh* mesh = new Mesh(Mesh::DrawMode::TRIANGLES);
        VertexBuffer* vData = mesh->addVertexBuffer(format, 24);

        auto position = mesh->getAccessor<glm::vec3>(AttributeType::POSITION);
        auto normal = mesh->getAccessor<glm::vec3>(AttributeType::NORMAL);

        for(size_t i = 0; i < vData->getNumVertices(); ++i) {
            position[i] = glm::make_vec3(vertices + i*3) * size;
            int side = i / 4;
            normal[i] = glm::make_vec3(normals[side]);
        }

        if(format.hasAttribute(AttributeType::TEXCOORD0)) {
            auto texCoord = mesh->getAccessor<glm::vec2>(AttributeType::TEXCOORD0);
            for(int side = 0; side < 6; ++side) {
                texCoord[side*4+0] = glm::vec2(0.0f, 0.0f);
                texCoord[side*4+1] = glm::vec2(0.0f, 1.0f);
                texCoord[side*4+2] = glm::vec2(1.0f, 1.0f);
                texCoord[side*4+3] = glm::vec2(1.0f, 0.0f);
            }
        }

        IndexBuffer* iData = mesh->setIndexBuffer(IndexBufferType::UI8, 36);

        uint8_t* indexBuffer = iData->getData<uint8_t>();
        for(int side = 0; side < 6; ++side) {
            for(int vertex = 0; vertex < 6; ++vertex) {
                indexBuffer[side*6+vertex] = 4*side + indices[vertex];
            }
        }

        //if(format.hasAttribute(AttributeType::TANGENT)) vData->calculateTangents();

        return mesh;
    }

    // Stacks represents the subdivision on the y axis (excluding the poles)
    Mesh* sphereMesh(float radius, int slices, int stacks, bool cubeProjectionTexCoords, const VertexFormat& format) {
        assert(slices > 3 && stacks > 2);
        Mesh* mesh = new Mesh(Mesh::DrawMode::TRIANGLES);
        mesh->addVertexBuffer(format, slices*stacks);

        auto position = mesh->getAccessor<glm::vec3>(AttributeType::POSITION);
        auto normal = mesh->getAccessor<glm::vec3>(AttributeType::NORMAL);
        auto texCoord = mesh->getAccessor<glm::vec2>(AttributeType::TEXCOORD0);

        /* This should probably be:
        auto normal = VertexAttributeAccessor();
        if(format.hasAttribute(AttributeType::NORMAL)) {
            normal = (*vData)[AttributeType::NORMAL];
        }

        That way there will not be a debug log message every time you create a sphere mesh,
        but I did it like this to show that it's possible to do it like this without everything going up in flames
         */

        int index = 0;
        for(int stack = 0; stack < stacks; ++stack) {
            float stackAngle = glm::pi<float>() / (stacks - 1) * stack;
            float xzRadius = glm::sin(stackAngle) * radius;
            float y = glm::cos(stackAngle) * radius;
            for(int slice = 0; slice < slices; ++slice) {
                float sliceAngle = 2.0f * glm::pi<float>() / (slices - 1) * slice;
                position[index] = glm::vec3(glm::cos(sliceAngle) * xzRadius, y, glm::sin(sliceAngle) * xzRadius);
                normal[index] = glm::normalize(position.get(index));
                if(cubeProjectionTexCoords) {
                    // http://www.gamedev.net/topic/443878-texture-lookup-in-cube-map/
                    glm::vec3 dir = normal.get(index);
                    glm::vec3 absDir = glm::abs(dir);
                    int majorDirIndex = 0;
                    if(absDir.x >= absDir.y && absDir.x >= absDir.z) majorDirIndex = 0;
                    if(absDir.y >= absDir.x && absDir.y >= absDir.z) majorDirIndex = 1;
                    if(absDir.z >= absDir.x && absDir.z >= absDir.y) majorDirIndex = 2;
                    float majorDirSign = 1.0f;
                    if(dir[majorDirIndex] < 0.0f) majorDirSign = -1.0f;

                    glm::vec3 v;
                    switch(majorDirIndex) {
                        case 0:
                            v = glm::vec3(-majorDirSign * dir.z, -dir.y, dir.x);
                            break;
                        case 1:
                            v = glm::vec3(dir.x, majorDirSign * dir.z, dir.y);
                            break;
                        case 2:
                            v = glm::vec3(majorDirSign * dir.x, -dir.y, dir.z);
                            break;
                        default:
                            assert(false);
                            break;
                    }

                    texCoord[index++] = glm::vec2((v.x/glm::abs(v.z) + 1.0f) / 2.0f, (v.y/glm::abs(v.z) + 1.0f) / 2.0f);
                } else {
                    texCoord[index++] = glm::vec2(sliceAngle / 2.0f / glm::pi<float>(), stackAngle / glm::pi<float>());
                }
            }
        }

        int triangles = 2 * (slices - 1) * (stacks - 1);

        IndexBuffer* iData = mesh->setIndexBuffer(IndexBufferType::UI16, triangles * 3);
        uint16_t* indexBuffer = iData->getData<uint16_t>();
        index = 0;
        for(int stack = 0; stack < stacks - 1; ++stack) {
            int firstStackVertex = stack * slices;
            for(int slice = 0; slice < slices - 1; ++slice) {
                int firstFaceVertex = firstStackVertex + slice;
                int nextVertex = firstFaceVertex + 1;
                indexBuffer[index++] = nextVertex + slices;
                indexBuffer[index++] = firstFaceVertex + slices;
                indexBuffer[index++] = firstFaceVertex;

                indexBuffer[index++] = nextVertex;
                indexBuffer[index++] = nextVertex + slices;
                indexBuffer[index++] = firstFaceVertex;
            }
        }

        //if(format.hasAttribute(AttributeType::TANGENT)) vData->calculateTangents();

        return mesh;
    }

    Mesh* planeMesh(float width, float height, int segmentsX, int segmentsY, const VertexFormat& format) {
        assert(segmentsX >= 1 && segmentsY >= 1);

        Mesh* mesh = new Mesh(Mesh::DrawMode::TRIANGLES);
        mesh->addVertexBuffer(format, (segmentsX+1)*(segmentsY+1));

        auto position = mesh->getAccessor<glm::vec3>(AttributeType::POSITION);
        auto normal = mesh->getAccessor<glm::vec3>(AttributeType::NORMAL);
        auto texCoord = mesh->getAccessor<glm::vec2>(AttributeType::TEXCOORD0);

        int index = 0;
        glm::vec2 size(width, height);
        for(int y = 0; y <= segmentsY; ++y) {
            for(int x = 0; x <= segmentsX; ++x) {
                glm::vec2 pos2D = glm::vec2((float)x / segmentsX, (float)y / segmentsY);
                texCoord[index] = pos2D;
                pos2D = pos2D * size - 0.5f * size;
                position[index] = glm::vec3(pos2D.x, 0.0f, pos2D.y);
                normal[index++] = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }

        IndexBuffer* iData = mesh->setIndexBuffer(IndexBufferType::UI16, segmentsX*segmentsY*2*3);
        uint16_t* indexBuffer = iData->getData<uint16_t>();
        index = 0;
        int perLine = segmentsX + 1;
        for(int y = 0; y < segmentsY; ++y) {
            for(int x = 0; x < segmentsX; ++x) {
                int start = x + y * perLine;
                indexBuffer[index++] = start;
                indexBuffer[index++] = start + perLine;
                indexBuffer[index++] = start + perLine + 1;

                indexBuffer[index++] = start + perLine + 1;
                indexBuffer[index++] = start + 1;
                indexBuffer[index++] = start;
            }
        }

        return mesh;
    }
}