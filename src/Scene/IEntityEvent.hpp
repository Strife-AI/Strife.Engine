#pragma once

#include "Physics/ColliderHandle.hpp"
#include "Scene/Entity.hpp"

struct IEntityEvent
{
    struct Metadata
    {

    };

    IEntityEvent(const Metadata* metadata)
        : _metadata(metadata)
    {
        
    }

    template<typename TEvent>
    bool Is(const TEvent*& outEvent) const
    {
        if (_metadata == TEvent::GetMetadata())
        {
            outEvent = static_cast<const TEvent*>(this);
            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename TEvent>
    const TEvent* Is() const
    {
        const TEvent* ev;
        return Is(ev) ? ev : nullptr;
    }

    const Metadata* GetMetadata() const
    {
        return _metadata;
    }

private:
    const Metadata* _metadata;
};

template<typename TEvent>
struct EntityEvent : IEntityEvent
{
    explicit EntityEvent()
        : IEntityEvent(&Metadata)
    {

    }

    static const Metadata* GetMetadata()
    {
        return &Metadata;
    }

private:
    static constexpr Metadata Metadata{ };
};

#define DEFINE_EVENT(_name) struct _name : EntityEvent<_name>
#define DEFINE_EMPTY_EVENT(_name) struct _name : EntityEvent<_name> { };

DEFINE_EVENT(ContactBeginEvent)
{
    ContactBeginEvent(const ColliderHandle& self_, const ColliderHandle& other_, const Vector2& normal_, b2Contact* contact_)
        : self(self_),
        other(other_),
        normal(normal_),
        contact(contact_)
    {
    }

    template<typename TEntity>
    bool OtherIs(TEntity*& outEntity) const
    {
        return other.OwningEntity()->Is<TEntity>(outEntity);
    }

    template<typename TEntity>
    bool OtherIs() const
    {
        return other.OwningEntity()->Is<TEntity>();
    }

    ColliderHandle self;
    ColliderHandle other;
    Vector2 normal;
    b2Contact* contact;
};

DEFINE_EVENT(ContactEndEvent)
{
    ContactEndEvent(const ColliderHandle& self_, const ColliderHandle& other_)
        : self(self_),
        other(other_)
    {
    }

    template<typename TEntity>
    bool OtherIs(TEntity*& outEntity) const
    {
        return other.OwningEntity()->Is<TEntity>(outEntity);
    }

    template<typename TEntity>
    bool OtherIs() const
    {
        return other.OwningEntity()->Is<TEntity>();
    }

    ColliderHandle self;
    ColliderHandle other;
};

DEFINE_EVENT(PreSolveEvent)
{
    PreSolveEvent(const ColliderHandle& self_, const ColliderHandle& other_, b2Contact* contact_)
        : self(self_),
        other(other_),
        contact(contact_)
    {
    }

    template<typename TEntity>
    bool OtherIs(TEntity * &outEntity) const
    {
        return other.OwningEntity()->Is<TEntity>(outEntity);
    }

    template<typename TEntity>
    bool OtherIs() const
    {
        return other.OwningEntity()->Is<TEntity>();
    }

    ColliderHandle self;
    ColliderHandle other;
    b2Contact* contact;
};

class Renderer;

DEFINE_EVENT(RenderEvent)
{
    RenderEvent(Renderer* renderer_)
        : renderer(renderer_)
    {

    }

    Renderer* renderer;
};

DEFINE_EMPTY_EVENT(RenderImguiEvent)
DEFINE_EMPTY_EVENT(UpdateEvent)
DEFINE_EMPTY_EVENT(EndOfUpdateEvent)
DEFINE_EMPTY_EVENT(FixedUpdateEvent)

/// <summary>
/// Sent after the scene has been loaded (usually after the first segment is done loading)
/// </summary>
DEFINE_EMPTY_EVENT(SceneLoadedEvent)

DEFINE_EMPTY_EVENT(EntityMovedEvent)
DEFINE_EMPTY_EVENT(EntityTeleportedEvent)

template<typename TEntity>
bool IsContactEndWith(const IEntityEvent& ev, TEntity*& outEntity)
{
    const ContactEndEvent* contactEnd;
    return ev.Is(contactEnd) && contactEnd->other.OwningEntity()->Is<TEntity>(outEntity);
}

template<typename TEntity>
bool IsContactBeginWith(const IEntityEvent& ev, TEntity*& outEntity)
{
    const ContactBeginEvent* contactBegin;
    return ev.Is(contactBegin) && contactBegin->other.OwningEntity()->Is<TEntity>(outEntity);
}

template<typename TEntity>
bool IsContactBeginWith(const IEntityEvent& ev)
{
    const ContactBeginEvent* contactBegin;
    return ev.Is(contactBegin) && contactBegin->other.OwningEntity()->Is<TEntity>();
}