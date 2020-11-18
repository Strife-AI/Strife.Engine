#include "PlayerEntity.hpp"


#include "InputService.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Physics/PathFinding.hpp"
#include "Renderer/Renderer.hpp"

int BitsNeeded(int value)
{
    int bits = 0;
    while(value > 0)
    {
        ++bits;
        value >>= 1;
    }

    return bits;
}

struct VarGroup
{
    ISyncVar* vars[32];
    bool changed[32];
    int changedCount = 0;
    int varCount = 0;
};

void WriteVarGroup(VarGroup* group, uint32 lastClientSequenceId, SLNet::BitStream& out)
{
    for (int i = 0; i < group->varCount; ++i)
    {
        if (!group->vars[i]->IsBool())
        {
            out.Write(group->changed[i]);

            if (group->changed[i])
            {
                group->vars[i]->WriteValueDeltaedFromSequence(lastClientSequenceId, out);
            }
        }
        else
        {
            // Writing a bit for whether a bool has changed just wastes a bit, so just write the bit
            group->vars[i]->WriteValueDeltaedFromSequence(lastClientSequenceId, out);
        }
    }
}

void WriteVars(ISyncVar* head, uint32 lastClientSequenceId, SLNet::BitStream& out)
{
    VarGroup frequent;
    VarGroup infrequent;

    for (auto var = head; var != nullptr; var = var->next)
    {
        VarGroup* group = var->frequency == SyncVarUpdateFrequency::Frequent
            ? &frequent
            : &infrequent;

        group->vars[group->varCount] = head;
        group->changed[group->varCount] = var->CurrentValueChangedFromSequence(lastClientSequenceId);
        group->changedCount += group->changed[group->varCount];
    }

    if (frequent.varCount == 0 && infrequent.varCount == 0)
    {
        out.Write0();
        return;
    }
    else
    {
        out.Write1();

        WriteVarGroup(&frequent, lastClientSequenceId, out);

        if (infrequent.varCount > 1)
        {
            out.Write(infrequent.changedCount > 0);
        }

        // Don't write any of the infrequent vars if none of them have changed
        if (infrequent.changedCount > 0)
        {
            WriteVarGroup(&infrequent, lastClientSequenceId, out);
        }
    }
}


void PlayerEntity::OnAdded(const EntityDictionary& properties)
{
    rigidBody = AddComponent<RigidBodyComponent>("rb", b2_dynamicBody);
    auto box = rigidBody->CreateBoxCollider(Dimensions());

    box->SetDensity(1);
    box->SetFriction(0);

    net = AddComponent<NetComponent>();

    scene->SendEvent(PlayerAddedToGame(this));

    scene->GetService<InputService>()->players.push_back(this);
}

void PlayerEntity::ReceiveEvent(const IEntityEvent& ev)
{
    if (ev.Is<SpawnedOnClientEvent>())
    {
        if (net->ownerClientId == scene->replicationManager.localClientId)
        {
            scene->GetCameraFollower()->FollowMouse();
            scene->GetCameraFollower()->CenterOn(Center());
            scene->GetService<InputService>()->activePlayer = this;
        }
    }
}

void PlayerEntity::ReceiveServerEvent(const IEntityEvent& ev)
{
    if (auto flowFieldReady = ev.Is<FlowFieldReadyEvent>())
    {
        net->flowField = flowFieldReady->result;
    }
    else if (auto moveTo = ev.Is<MoveToEvent>())
    {
        scene->GetService<PathFinderService>()->RequestFlowField(Center(), moveTo->position, this);
        state = PlayerState::Moving;
    }
    else if (auto attack = ev.Is<AttackEvent>())
    {
        attackTarget = attack->entity;
        updateTargetTimer = 0;
        state = PlayerState::Attacking;
    }
}

void PlayerEntity::OnDestroyed()
{
    scene->SendEvent(PlayerRemovedFromGame(this));
}

void PlayerEntity::Render(Renderer* renderer)
{
    auto position = Center();

    Color c[3] = {
        Color::CornflowerBlue(),
        Color::Green(),
        Color::Orange()
    };

    auto color = c[net->ownerClientId];

    renderer->RenderRectangle(Rectangle(position - Dimensions() / 2, Dimensions()), color, -0.99);

    Vector2 healthBarSize(32, 4);
    renderer->RenderRectangle(Rectangle(
        Center() - Vector2(0, 20) - healthBarSize / 2,
        Vector2(healthBarSize.x * health / 100, healthBarSize.y)),
        Color::White(),
        -1);

    if (showAttack)
    {
        renderer->RenderLine(Center(), attackPosition, Color::Red(), -1);
    }
}

void PlayerEntity::ServerFixedUpdate(float deltaTime)
{
    auto client = net;

    attackCoolDown -= deltaTime;

    if(state == PlayerState::Attacking)
    {
        Entity* target;
        RaycastResult hitResult;
        if(attackTarget.TryGetValue(target)
            && (target->Center() - Center()).Length() < 200
            && scene->Raycast(Center(), target->Center(), hitResult)
            && hitResult.handle.OwningEntity() == target)
        {
            rigidBody->SetVelocity({ 0, 0 });

            if (attackCoolDown <= 0)
            {
                PlayerEntity* player;
                if (target->Is<PlayerEntity>(player))
                {
                    player->health -= 10;

                    if (player->health <= 0)
                    {
                        player->SetCenter({ -1000, -1000 });
                        attackTarget = nullptr;
                    }

                    attackCoolDown = 3;
                    showAttack = true;
                    StartTimer(0.3, [=]
                    {
                        showAttack = false;
                    });
                }
            }

            return;
        }
    }

    if (client->flowField != nullptr)
    {
        Vector2 velocity;

        Vector2 points[4];
        client->owner->Bounds().GetPoints(points);

        bool useBeeLine = true;
        for (auto p : points)
        {
            RaycastResult result;
            if (scene->Raycast(p, client->flowField->target, result)
                && (state != PlayerState::Attacking || result.handle.OwningEntity() != attackTarget.GetValueOrNull()))
            {
                useBeeLine = false;
                break;
            }
        }

        if (useBeeLine)
        {
            velocity = (client->flowField->target - client->owner->Center()).Normalize() * 200;
        }
        else
        {
            velocity = client->flowField->GetFilteredFlowDirection(client->owner->Center() - Vector2(16, 16)) * 200;

        }

        velocity = client->owner->GetComponent<RigidBodyComponent>()->GetVelocity().SmoothDamp(
            velocity,
            client->acceleration,
            0.05,
            Scene::PhysicsDeltaTime);

        if ((client->owner->Center() - client->flowField->target).Length() < 200 * Scene::PhysicsDeltaTime)
        {
            velocity = { 0, 0 };
            client->flowField = nullptr;
        }

        client->owner->GetComponent<RigidBodyComponent>()->SetVelocity(velocity);
        //Renderer::DrawDebugLine({ client->owner->Center(), client->owner->Center() + velocity, Color::Red() });
    }
}

void PlayerEntity::ServerUpdate(float deltaTime)
{
    if (state == PlayerState::Attacking)
    {
        Entity* target;
        if (attackTarget.TryGetValue(target))
        {
            attackPosition = target->Center();
            updateTargetTimer -= deltaTime;
            if (updateTargetTimer <= 0)
            {
                scene->GetService<PathFinderService>()->RequestFlowField(Center(), target->Center(), this);
                updateTargetTimer = 2;
            }
            else if (net->flowField != nullptr)
            {
                net->flowField->target = target->Center();
            }
        }
        else
        {
            state = PlayerState::None;
            net->flowField = nullptr;
        }
    }
}

void PlayerEntity::SetMoveDirection(Vector2 direction)
{
    rigidBody->SetVelocity(direction);
}

void PlayerEntity::DoNetSerialize(NetSerializer& serializer)
{
    serializer.Add(showAttack);
    serializer.Add(attackPosition);
    if(serializer.Add(health))
    {

    }
}
