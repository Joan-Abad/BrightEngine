#pragma once

#include "brightengine/Handle.h"
#include "brightengine/scene/Renderable.h"
#include "brightengine/scene/Transform.h"

#include <vector>

namespace brightengine
{
    struct EntityTag {};
    using Entity = Handle<EntityTag>;

    // Minimal ECS: Transform and Renderable live in their own dense,
    // contiguous arrays (not scattered per-object like Actor/Component),
    // indexed by entity id. Real ECS libraries support entities with only
    // some of the possible components via sparse sets/archetypes -- we
    // don't need that yet, so every entity here always has both. No entity
    // destruction yet either; add it once something actually needs to
    // remove an entity mid-run.
    class Scene
    {
    public:
        Entity CreateEntity()
        {
            m_transforms.emplace_back();
            m_renderables.emplace_back();
            return Entity{ m_nextEntityId++ };
        }

        Transform& GetTransform(Entity entity) { return m_transforms[entity.id - 1]; }
        Renderable& GetRenderable(Entity entity) { return m_renderables[entity.id - 1]; }

        const std::vector<Transform>& GetTransforms() const { return m_transforms; }
        const std::vector<Renderable>& GetRenderables() const { return m_renderables; }

    private:
        std::vector<Transform> m_transforms;
        std::vector<Renderable> m_renderables;
        uint32_t m_nextEntityId = 1;
    };
}
