#pragma once

#include <vector>
#include <cstdio>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ngn {
    class SceneNode {
    public:
        using Id = uint32_t;

    protected:
        glm::vec3 mPosition, mScale;
        glm::quat mQuaternion;
        glm::mat4 mMatrix;

        void updateTRSFromMatrix();
        virtual void dirty() {mMatrixDirty = true;}

    private:
        Id mId;

        SceneNode* mParent;
        std::vector<SceneNode*> mChildren;

        bool mMatrixDirty;

    public:
        static Id nextId;
        static std::map<Id, SceneNode*> nodeIdMap;

        static SceneNode* getById(Id id) {
            auto it = nodeIdMap.find(id);
            if(it != nodeIdMap.end()) {
                return it->second;
            } else {
                return nullptr;
            }
        }

        SceneNode() : mPosition(0.0f, 0.0f, 0.0f), mScale(1.0f, 1.0f, 1.0f), mQuaternion(),
                mParent(nullptr), mMatrixDirty(true) {
            nodeIdMap[mId = nextId++] = this;
        }

        ~SceneNode() {
            if(mParent != nullptr) mParent->remove(this);
        }

        // Id etc.
        Id getId() const {return mId;}
        // if you use setId, please make sure SceneNode::nextId has a valid value after it
        // probably something like: SceneNode::nextId == std::max(SceneNode::nextId + 1, theIdISetTo);
        void setId(Id id) {
            nodeIdMap.erase(mId);
            nodeIdMap[mId = id] = this;
        }

        // Hierarchy
        SceneNode* getParent() {return mParent;}

        // you may add a node twice to the graph, which is not intended, but the overhead of checking is undesirable
        void add(SceneNode* obj) {
            obj->mParent = this;
            mChildren.push_back(obj);
        }

        void remove(SceneNode* obj) {
            auto it = mChildren.begin();
            while(it != mChildren.end()) {
                if(*it == obj) {
                    (*it)->mParent = nullptr;
                    it = mChildren.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Transforms
        void setPosition(const glm::vec3& position) {mPosition = position; dirty();}
        glm::vec3 getPosition() const {return mPosition;}

        void setScale(const glm::vec3& scale) {mScale = scale; dirty();}
        glm::vec3 getScale() const {return mScale;}

        void setQuaternion(const glm::quat& quat) {mQuaternion = quat; dirty();}
        glm::quat getQuaternion() const {return mQuaternion;}

        void rotate(const glm::quat& quat) {mQuaternion = glm::normalize(quat * mQuaternion); dirty();}
        void rotate(float angleRadians, const glm::vec3& axis) {
            // this normalization is not necessary mathematically, but technically (because of floating point numbers) it is
            mQuaternion = glm::normalize(glm::angleAxis(angleRadians, axis) * mQuaternion);
            dirty();
        }

        void rotateWorld(float angleRadians, const glm::vec3& worldAxis) {
            rotate(angleRadians, mQuaternion * worldAxis);
        }

        // For FPS camera ust localDirToWorld, then set y = 0 and renormalize
        glm::vec3 localDirToWorld(const glm::vec3& vec) const {return glm::conjugate(mQuaternion) * vec;}
        glm::vec3 getForward() const {return localDirToWorld(glm::vec3(0.0f, 0.0f, -1.0f));}
        glm::vec3 getRight() const {return localDirToWorld(glm::vec3(1.0f, 0.0f, 0.0f));}
        glm::vec3 getUp() const {return localDirToWorld(glm::vec3(0.0f, 1.0f, 0.0f));}

        glm::mat3 getLocalSystem() const {
            glm::mat3 ret;
            ret[0] = getRight();
            ret[1] = getUp();
            ret[2] = getForward();
            return ret;
        }

        glm::mat4 getMatrix() {
            if(mMatrixDirty) {
                mMatrix = glm::scale(glm::translate(glm::mat4(), mPosition) * glm::mat4_cast(mQuaternion), mScale);
                mMatrixDirty = false;
            }
            return mMatrix;
        }

        glm::mat4 getWorldMatrix() {
            if(mParent)
                return mParent->getWorldMatrix() * getMatrix();
            else
                return getMatrix();
        }

        void setMatrix(const glm::mat4& matrix, bool updateTRS = true) {
            mMatrix = matrix;
            mMatrixDirty = false;
            if(updateTRS) updateTRSFromMatrix();
        }

        // this works just as gluLookAt, so may put in "world space up" (this might not work sometimes, but makes everything a lot easier most of the time)
        // it also means, that the negative z-axis is aligned to face "at"
        void lookAt(const glm::vec3& pos, const glm::vec3& at, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) {
            mPosition = pos;
            mQuaternion = glm::normalize(glm::quat_cast(glm::lookAt(pos, at, up)));
            dirty();
        }

        void lookAt(const glm::vec3& at, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) {
            lookAt(mPosition, at, up);
        }
    };

    using Scene = SceneNode;
}