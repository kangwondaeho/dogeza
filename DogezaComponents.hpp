#pragma once
#include "ObjectBase.hpp"
#include "Material.hpp"
#include "MeshRenderer.hpp"
#include "DogezaTypes.hpp"

inline float ClampF(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

struct ProjectedPoint
{
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
    float laneHalf = 1.0f;
    float relativeZ = 0.0f;
    bool visible = true;
};

class MapRandomizerComponent : public Component
{
private:
    std::vector<MapRule> maps;
    int selectedIndex = 0;

public:
    MapRandomizerComponent()
    {
        maps.push_back({ MapType::STOP, "MAP A", "Immediate Stop", 0.0f });
        maps.push_back({ MapType::SHORT_INERTIA, "MAP B", "1.5 sec Inertia", 1.5f });
        maps.push_back({ MapType::LONG_INERTIA, "MAP C", "3.0 sec Inertia", 3.0f });
    }

    void SelectRandomMap()
    {
        selectedIndex = rand() % (int)maps.size();
    }

    const MapRule& CurrentMap() const
    {
        return maps[selectedIndex];
    }

    float GetInertiaSeconds() const
    {
        return CurrentMap().inertiaSeconds;
    }

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}
    void Update(float dt) override {}
    void Render(GraphicsContext* gfx) override {}
};


class DepthTargetComponent : public Component
{
private:
    float receiverZ = 720.0f;

public:
    void Randomize()
    {
        receiverZ = 640.0f + (float)(rand() % 420);
    }

    float GetTargetZ() const { return receiverZ; }
    void SetTargetZ(float z) { receiverZ = z; }

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}
    void Update(float dt) override {}
    void Render(GraphicsContext* gfx) override {}
};

class ProneSlideComponent : public Component
{
private:
    float startZ = 44.0f;
    float worldZ = 44.0f;
    float runVelocity = 260.0f;
    float velocity = 0.0f;
    float slideStartVelocity = 0.0f;
    float slideTimer = 0.0f;
    bool approaching = false;
    bool prone = false;
    bool slideFinished = false;

public:
    void Reset()
    {
        worldZ = startZ;
        velocity = 0.0f;
        slideStartVelocity = 0.0f;
        slideTimer = 0.0f;
        approaching = false;
        prone = false;
        slideFinished = false;
    }

    void StartApproach()
    {
        approaching = true;
        prone = false;
        slideFinished = false;
        velocity = runVelocity;
    }

    void StartProneSlide(float inertiaSeconds)
    {
        approaching = false;
        prone = true;
        slideTimer = 0.0f;
        slideStartVelocity = runVelocity;

        if (inertiaSeconds <= 0.0f)
        {
            velocity = 0.0f;
            slideFinished = true;
        }
        else
        {
            velocity = slideStartVelocity;
            slideFinished = false;
        }
    }

    void ForceStop()
    {
        velocity = 0.0f;
        approaching = false;
        slideFinished = true;
    }

    float GetWorldZ() const { return worldZ; }
    float GetVelocity() const { return velocity; }
    float GetRunVelocity() const { return runVelocity; }
    float GetSlideTimer() const { return slideTimer; }
    bool IsProne() const { return prone; }
    bool IsApproaching() const { return approaching; }
    bool IsSlideFinished() const { return prone && slideFinished; }
    bool IsMoving() const { return approaching || (prone && !slideFinished); }

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (approaching)
        {
            worldZ += runVelocity * dt;
            velocity = runVelocity;
        }
    }

    void UpdateSlide(float dt, float inertiaSeconds)
    {
        if (!prone || slideFinished) return;

        if (inertiaSeconds <= 0.0f)
        {
            velocity = 0.0f;
            slideFinished = true;
            return;
        }

        slideTimer += dt;
        float t = ClampF(slideTimer / inertiaSeconds, 0.0f, 1.0f);
        velocity = slideStartVelocity * (1.0f - t);
        worldZ += velocity * dt;

        if (slideTimer >= inertiaSeconds)
        {
            velocity = 0.0f;
            slideFinished = true;
        }
    }

    void Render(GraphicsContext* gfx) override {}
};

class FollowCameraComponent : public Component
{
private:
    ProneSlideComponent* player = nullptr;
    float cameraZ = 0.0f;
    float followDistance = 160.0f;
    float followLerp = 6.5f;
    float maxDepth = 1050.0f;

    float shakeStrength = 0.0f;
    float impactStrength = 0.0f;
    float shakeTime = 0.0f;
    float shakeX = 0.0f;
    float shakeY = 0.0f;

public:
    FollowCameraComponent(ProneSlideComponent* playerMotion)
        : player(playerMotion) {}

    void Reset()
    {
        cameraZ = 0.0f;
        shakeStrength = 0.0f;
        impactStrength = 0.0f;
        shakeTime = 0.0f;
        shakeX = 0.0f;
        shakeY = 0.0f;
    }

    float GetCameraZ() const { return cameraZ; }
    float GetShakeX() const { return shakeX; }
    float GetShakeY() const { return shakeY; }

    void SetShakeStrength(float strength)
    {
        shakeStrength = ClampF(strength, 0.0f, 1.0f);
    }

    void TriggerImpactShake(float strength)
    {
        if (strength > impactStrength)
        {
            impactStrength = ClampF(strength, 0.0f, 1.75f);
        }
    }

    ProjectedPoint Project(float worldX, float worldZ) const
    {
        float relZ = worldZ - cameraZ;
        float t = ClampF(relZ / maxDepth, 0.0f, 1.0f);

        float nearLaneHalf = 0.95f;
        float farLaneHalf = 0.15f;
        float laneHalf = nearLaneHalf + (farLaneHalf - nearLaneHalf) * t;

        float bottomY = -0.86f;
        float horizonY = 0.46f;
        float screenY = bottomY + (horizonY - bottomY) * t;

        float screenScale = 1.12f + (0.28f - 1.12f) * t;
        float screenX = worldX * (laneHalf / nearLaneHalf);

        ProjectedPoint p;
        p.x = screenX + shakeX;
        p.y = screenY + shakeY;
        p.scale = screenScale;
        p.laneHalf = laneHalf;
        p.relativeZ = relZ;
        p.visible = relZ > -120.0f && relZ < maxDepth + 120.0f;
        return p;
    }

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!player) return;
        float targetCameraZ = player->GetWorldZ() - followDistance;
        if (targetCameraZ < 0.0f) targetCameraZ = 0.0f;
        float a = 1.0f - expf(-followLerp * dt);
        cameraZ += (targetCameraZ - cameraZ) * a;

        shakeTime += dt;

        float targetX = 0.0f;
        float targetY = 0.0f;

        impactStrength -= dt * 2.65f;
        if (impactStrength < 0.0f) impactStrength = 0.0f;

        float combinedStrength = shakeStrength;
        if (impactStrength > combinedStrength) combinedStrength = impactStrength;

        if (combinedStrength > 0.001f)
        {
            float ampX = 0.018f * combinedStrength;
            float ampY = 0.012f * combinedStrength;

            targetX =
                sinf(shakeTime * 42.0f) * ampX +
                sinf(shakeTime * 91.0f + 1.7f) * ampX * 0.45f;

            targetY =
                cosf(shakeTime * 37.0f + 0.6f) * ampY +
                sinf(shakeTime * 83.0f + 2.1f) * ampY * 0.42f;
        }

        float fade = 1.0f - expf(-10.0f * dt);
        shakeX += (targetX - shakeX) * fade;
        shakeY += (targetY - shakeY) * fade;
    }

    void Render(GraphicsContext* gfx) override {}
};


class BackgroundZoomComponent : public Component
{
private:
    TextureMaterial* material = nullptr;
    FollowCameraComponent* camera = nullptr;
    MapRandomizerComponent* map = nullptr;
    Texture* stopTexture = nullptr;
    Texture* shortTexture = nullptr;
    Texture* longTexture = nullptr;
    ID3D11Buffer* cBuffer = nullptr;
    ID3D11Buffer* vBuffer = nullptr;
    float vanishU = 0.50f;
    float vanishV = 0.40f;
    float zoomBase = 1.0f;
    float zoomFromCamera = 0.00135f;
    float maxZoom = 2.15f;
    UINT vertexCount = 6;

public:
    BackgroundZoomComponent(TextureMaterial* mat, FollowCameraComponent* cam,
        MapRandomizerComponent* mapComp,
        Texture* stopTex,
        Texture* shortTex,
        Texture* longTex)
        : material(mat), camera(cam), map(mapComp),
        stopTexture(stopTex), shortTexture(shortTex), longTexture(longTex) {}

    ~BackgroundZoomComponent()
    {
        if (cBuffer) cBuffer->Release();
        if (vBuffer) vBuffer->Release();
    }

    void Start(GraphicsContext* gfx) override
    {
        D3D11_BUFFER_DESC cbd = {};
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ConstantBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        gfx->Device->CreateBuffer(&cbd, nullptr, &cBuffer);

        D3D11_BUFFER_DESC vbd = {};
        vbd.Usage = D3D11_USAGE_DEFAULT;
        vbd.ByteWidth = sizeof(Vertex) * vertexCount;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        gfx->Device->CreateBuffer(&vbd, nullptr, &vBuffer);
    }

    void Input() override {}

    void Update(float dt) override
    {
        if (!material || !map) return;

        Texture* desired = stopTexture;

        MapType type = map->CurrentMap().type;
        if (type == MapType::STOP)
        {
            desired = stopTexture;
        }
        else if (type == MapType::SHORT_INERTIA)
        {
            desired = shortTexture ? shortTexture : stopTexture;
        }
        else if (type == MapType::LONG_INERTIA)
        {
            desired = longTexture ? longTexture : stopTexture;
        }

        if (desired && material->GetTexture() != desired)
        {
            material->SetTexture(desired);
        }
    }

    void BuildVertices(Vertex outVerts[6]) const
    {
        float cameraZ = camera ? camera->GetCameraZ() : 0.0f;
        float zoom = zoomBase + cameraZ * zoomFromCamera;
        zoom = ClampF(zoom, 1.0f, maxZoom);

        float centerU = vanishU;
        float centerV = vanishV - (zoom - 1.0f) * 0.030f;
        centerV = ClampF(centerV, 0.30f, 0.45f);

        float halfU = 0.5f / zoom;
        float halfV = 0.5f / zoom;

        float u0 = centerU - halfU;
        float u1 = centerU + halfU;
        float v0 = centerV - halfV;
        float v1 = centerV + halfV;

        if (u0 < 0.0f) { u1 -= u0; u0 = 0.0f; }
        if (v0 < 0.0f) { v1 -= v0; v0 = 0.0f; }
        if (u1 > 1.0f) { u0 -= (u1 - 1.0f); u1 = 1.0f; }
        if (v1 > 1.0f) { v0 -= (v1 - 1.0f); v1 = 1.0f; }

        u0 = ClampF(u0, 0.0f, 1.0f);
        u1 = ClampF(u1, 0.0f, 1.0f);
        v0 = ClampF(v0, 0.0f, 1.0f);
        v1 = ClampF(v1, 0.0f, 1.0f);

        float sx = camera ? camera->GetShakeX() : 0.0f;
        float sy = camera ? camera->GetShakeY() : 0.0f;

        // Slight overscan prevents black borders while the camera shakes.
        outVerts[0] = { { -1.04f + sx,  1.04f + sy, 0.0f }, { u0, v0, 0.0f, 1.0f } };
        outVerts[1] = { {  1.04f + sx,  1.04f + sy, 0.0f }, { u1, v0, 0.0f, 1.0f } };
        outVerts[2] = { { -1.04f + sx, -1.04f + sy, 0.0f }, { u0, v1, 0.0f, 1.0f } };

        outVerts[3] = { {  1.04f + sx, -1.04f + sy, 0.0f }, { u1, v1, 0.0f, 1.0f } };
        outVerts[4] = { { -1.04f + sx, -1.04f + sy, 0.0f }, { u0, v1, 0.0f, 1.0f } };
        outVerts[5] = { {  1.04f + sx,  1.04f + sy, 0.0f }, { u1, v0, 0.0f, 1.0f } };
    }

    float CurrentZoom() const
    {
        float cameraZ = camera ? camera->GetCameraZ() : 0.0f;
        float zoom = zoomBase + cameraZ * zoomFromCamera;
        return ClampF(zoom, 1.0f, maxZoom);
    }

    void CurrentCrop(float& u0, float& v0, float& u1, float& v1) const
    {
        float zoom = CurrentZoom();
        float centerU = vanishU;
        float centerV = vanishV - (zoom - 1.0f) * 0.030f;
        centerV = ClampF(centerV, 0.30f, 0.45f);

        float halfU = 0.5f / zoom;
        float halfV = 0.5f / zoom;

        u0 = centerU - halfU;
        u1 = centerU + halfU;
        v0 = centerV - halfV;
        v1 = centerV + halfV;

        if (u0 < 0.0f) { u1 -= u0; u0 = 0.0f; }
        if (v0 < 0.0f) { v1 -= v0; v0 = 0.0f; }
        if (u1 > 1.0f) { u0 -= (u1 - 1.0f); u1 = 1.0f; }
        if (v1 > 1.0f) { v0 -= (v1 - 1.0f); v1 = 1.0f; }

        u0 = ClampF(u0, 0.0f, 1.0f);
        u1 = ClampF(u1, 0.0f, 1.0f);
        v0 = ClampF(v0, 0.0f, 1.0f);
        v1 = ClampF(v1, 0.0f, 1.0f);
    }

    float TrackVFromWorldZ(float worldZ) const
    {
        // This maps the game depth value onto the original background image path.
        // Farther worldZ means the anchor point is closer to the building entrance.
        float t = ClampF((worldZ - 44.0f) / 1120.0f, 0.0f, 1.0f);
        return 0.92f + (0.42f - 0.92f) * t;
    }

    ProjectedPoint ProjectTrackZ(float worldZ, float laneX = 0.0f) const
    {
        float u0, v0, u1, v1;
        CurrentCrop(u0, v0, u1, v1);

        float baseV = TrackVFromWorldZ(worldZ);
        float laneT = ClampF((baseV - 0.42f) / (0.92f - 0.42f), 0.0f, 1.0f);
        float laneWidthUv = 0.10f + 0.32f * laneT;
        float baseU = 0.50f + laneX * laneWidthUv;

        float cropW = u1 - u0;
        float cropH = v1 - v0;

        ProjectedPoint p;
        p.x = ((baseU - u0) / cropW) * 2.0f - 1.0f;
        p.y = 1.0f - ((baseV - v0) / cropH) * 2.0f;

        if (camera)
        {
            p.x += camera->GetShakeX();
            p.y += camera->GetShakeY();
        }

        p.relativeZ = worldZ - (camera ? camera->GetCameraZ() : 0.0f);
        p.laneHalf = 0.20f + 0.55f * laneT;

        float zoom = CurrentZoom();
        float perspectiveScale = 0.32f + 0.95f * laneT;
        p.scale = perspectiveScale * zoom;

        p.visible = (baseU >= u0 - 0.08f && baseU <= u1 + 0.08f &&
            baseV >= v0 - 0.08f && baseV <= v1 + 0.08f);
        return p;
    }

    void Render(GraphicsContext* gfx) override
    {
        if (!gfx || !material || !cBuffer || !vBuffer) return;

        Vertex verts[6];
        BuildVertices(verts);
        gfx->ImmediateContext->UpdateSubresource(vBuffer, 0, nullptr, verts, 0, 0);

        material->Bind(gfx->ImmediateContext);

        ConstantBuffer cb;
        cb.matWorld = XMMatrixTranspose(XMMatrixIdentity());
        gfx->ImmediateContext->UpdateSubresource(cBuffer, 0, nullptr, &cb, 0, 0);
        gfx->ImmediateContext->VSSetConstantBuffers(0, 1, &cBuffer);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        gfx->ImmediateContext->IASetVertexBuffers(0, 1, &vBuffer, &stride, &offset);
        gfx->ImmediateContext->Draw(vertexCount, 0);
    }
};

class PlayerVisualComponent : public Component
{
private:
    FollowCameraComponent* camera = nullptr;
    ProneSlideComponent* player = nullptr;
    MeshRenderer* renderer = nullptr;
    Material* standingMaterial = nullptr;
    Material* proneMaterial = nullptr;

public:
    PlayerVisualComponent(FollowCameraComponent* cam, ProneSlideComponent* playerMotion,
        MeshRenderer* meshRenderer = nullptr,
        Material* standingMat = nullptr,
        Material* proneMat = nullptr)
        : camera(cam), player(playerMotion), renderer(meshRenderer), standingMaterial(standingMat), proneMaterial(proneMat) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!camera || !player) return;

        ProjectedPoint p = camera->Project(0.0f, player->GetWorldZ());
        if (!p.visible)
        {
            pOwner->scale = { 0.0f, 0.0f, 1.0f };
            return;
        }

        bool prone = player->IsProne();
        if (renderer)
        {
            if (prone && proneMaterial) renderer->pMaterial = proneMaterial;
            else if (!prone && standingMaterial) renderer->pMaterial = standingMaterial;
        }

        float baseW = prone ? 0.34f : 0.12f;
        float baseH = prone ? 0.08f : 0.30f;
        float w = baseW * p.scale;
        float h = baseH * p.scale;

        pOwner->pos = { p.x, p.y + h * 0.5f, 0.0f };
        pOwner->scale = { w, h, 1.0f };
    }

    void Render(GraphicsContext* gfx) override {}
};

class ReceiverVisualComponent : public Component
{
private:
    BackgroundZoomComponent* background = nullptr;
    DepthTargetComponent* target = nullptr;

public:
    ReceiverVisualComponent(BackgroundZoomComponent* bg, DepthTargetComponent* depthTarget)
        : background(bg), target(depthTarget) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!background || !target) return;

        ProjectedPoint p = background->ProjectTrackZ(target->GetTargetZ());
        if (!p.visible)
        {
            pOwner->scale = { 0.0f, 0.0f, 1.0f };
            return;
        }

        float w = 0.14f * p.scale;
        float h = 0.34f * p.scale;
        pOwner->pos = { p.x, p.y + h * 0.5f, 0.0f };
        pOwner->scale = { w, h, 1.0f };
    }

    void Render(GraphicsContext* gfx) override {}
};



class CameraShakeComponent : public Component
{
private:
    FollowCameraComponent* camera = nullptr;
    ProneSlideComponent* player = nullptr;
    DepthTargetComponent* target = nullptr;

    float startDistance = 260.0f;
    float maxDistance = 20.0f;
    float maxStrength = 1.0f;

public:
    CameraShakeComponent(FollowCameraComponent* cam, ProneSlideComponent* playerMotion, DepthTargetComponent* depthTarget)
        : camera(cam), player(playerMotion), target(depthTarget) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!camera || !player || !target) return;

        float strength = 0.0f;

        // Shake only while the player is moving. It disappears when the player stops and the result appears.
        if (player->IsMoving())
        {
            float distance = target->GetTargetZ() - player->GetWorldZ();

            if (distance > 0.0f && distance < startDistance)
            {
                float denom = startDistance - maxDistance;
                if (fabsf(denom) < 0.0001f) denom = 0.0001f;

                float t = ClampF((startDistance - distance) / denom, 0.0f, 1.0f);

                // Smoothstep curve: weak far away, strong near the receiver.
                strength = t * t * (3.0f - 2.0f * t);
                strength *= maxStrength;

                if (player->IsProne())
                {
                    strength *= 1.25f;
                }
            }
        }

        camera->SetShakeStrength(strength);
    }

    void Render(GraphicsContext* gfx) override {}
};


class ReceiverProximityCueComponent : public Component
{
private:
    BackgroundZoomComponent* background = nullptr;
    DepthTargetComponent* target = nullptr;
    ProneSlideComponent* player = nullptr;

    float appearDistance = 220.0f;
    float fullStrengthDistance = 40.0f;
    float baseScale = 0.34f;
    float extraScale = 0.12f;
    float pulseAmplitude = 0.05f;
    float pulseSpeed = 3.2f;
    float rotationSpeed = 0.65f;
    float startRotation = 0.0f;
    float yOffsetFactor = 0.50f;
    float animTime = 0.0f;

public:
    ReceiverProximityCueComponent(BackgroundZoomComponent* bg, DepthTargetComponent* depthTarget,
        ProneSlideComponent* playerMotion,
        float showDist = 220.0f, float strongDist = 40.0f,
        float size0 = 0.34f, float sizeAdd = 0.12f,
        float pulseAmp = 0.05f, float pulseHz = 3.2f,
        float rotSpeed = 0.65f, float rot0 = 0.0f, float yFactor = 0.50f)
        : background(bg), target(depthTarget), player(playerMotion),
        appearDistance(showDist), fullStrengthDistance(strongDist),
        baseScale(size0), extraScale(sizeAdd), pulseAmplitude(pulseAmp),
        pulseSpeed(pulseHz), rotationSpeed(rotSpeed), startRotation(rot0), yOffsetFactor(yFactor) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!background || !target || !player) return;
        animTime += dt;

        ProjectedPoint p = background->ProjectTrackZ(target->GetTargetZ());
        if (!p.visible)
        {
            pOwner->scale = { 0.0f, 0.0f, 1.0f };
            return;
        }

        float distance = target->GetTargetZ() - player->GetWorldZ();
        if (distance <= 0.0f)
        {
            pOwner->scale = { 0.0f, 0.0f, 1.0f };
            return;
        }

        if (distance > appearDistance)
        {
            pOwner->scale = { 0.0f, 0.0f, 1.0f };
            return;
        }

        float denom = (appearDistance - fullStrengthDistance);
        if (fabsf(denom) < 0.0001f) denom = 0.0001f;
        float strength = ClampF((appearDistance - distance) / denom, 0.0f, 1.0f);

        // Make the halo feel more dramatic as the player gets closer.
        float pulse = 1.0f + pulseAmplitude * (0.35f + 0.65f * strength) * sinf(animTime * pulseSpeed + startRotation * 1.7f);
        float size = (baseScale + extraScale * strength) * p.scale * pulse;

        pOwner->rot.z = startRotation + animTime * rotationSpeed * (0.35f + strength * 1.45f);
        pOwner->pos = { p.x, p.y + size * yOffsetFactor, 0.0f };
        pOwner->scale = { size, size, 1.0f };
    }

    void Render(GraphicsContext* gfx) override {}
};

class DistanceMeterComponent : public Component
{
private:
    ProneSlideComponent* player = nullptr;
    DepthTargetComponent* target = nullptr;
    ColorMaterial* material = nullptr;
    float maxWidth = 0.82f;
    float leftX = -0.41f;
    float y = -0.82f;

public:
    DistanceMeterComponent(ProneSlideComponent* playerMotion, DepthTargetComponent* depthTarget, ColorMaterial* mat)
        : player(playerMotion), target(depthTarget), material(mat) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!player || !target || !material) return;

        float distance = target->GetTargetZ() - player->GetWorldZ();
        float close = ClampF(1.0f - distance / 260.0f, 0.0f, 1.0f);
        float width = maxWidth * close;

        if (distance <= 0.0f)
        {
            width = maxWidth;
            material->SetColor(XMFLOAT4(1.0f, 0.10f, 0.08f, 0.85f));
        }
        else if (distance <= 25.0f)
        {
            material->SetColor(XMFLOAT4(1.0f, 0.86f, 0.20f, 0.86f));
        }
        else if (distance <= 60.0f)
        {
            material->SetColor(XMFLOAT4(0.45f, 0.72f, 1.0f, 0.78f));
        }
        else if (distance <= 120.0f)
        {
            material->SetColor(XMFLOAT4(0.36f, 0.82f, 0.45f, 0.68f));
        }
        else
        {
            material->SetColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 0.38f));
        }

        if (width < 0.015f) width = 0.015f;
        pOwner->pos = { leftX + width * 0.5f, y, 0.0f };
        pOwner->scale = { width, 0.030f, 1.0f };
    }

    void Render(GraphicsContext* gfx) override {}
};

class JudgeComponent : public Component
{
private:
    ProneSlideComponent* player = nullptr;
    DepthTargetComponent* target = nullptr;
    float error = 0.0f;
    JudgeResult result = JudgeResult::NONE;

public:
    JudgeComponent(ProneSlideComponent* playerMotion, DepthTargetComponent* depthTarget)
        : player(playerMotion), target(depthTarget) {}

    JudgeResult Evaluate()
    {
        if (!player || !target)
        {
            result = JudgeResult::FAIL;
            return result;
        }

        // Higher score means the player stopped as far forward as possible
        // while still remaining in front of the receiver.
        // If the player touches or passes the receiver, it is treated as a failure.
        error = target->GetTargetZ() - player->GetWorldZ();

        if (error <= 0.0f)       result = JudgeResult::FAIL;
        else if (error <= 25.0f) result = JudgeResult::PERFECT;
        else if (error <= 60.0f) result = JudgeResult::GREAT;
        else if (error <= 120.0f) result = JudgeResult::GOOD;
        else                     result = JudgeResult::FAIL;

        return result;
    }

    void Reset()
    {
        error = 0.0f;
        result = JudgeResult::NONE;
    }

    float GetError() const { return error; }
    JudgeResult GetResult() const { return result; }

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}
    void Update(float dt) override {}
    void Render(GraphicsContext* gfx) override {}
};

class ScoreComponent : public Component
{
private:
    int currentScore = 0;
    int highScore = 0;
    int lastScore = 0;

public:
    void AddScore(JudgeResult result)
    {
        lastScore = ScoreOf(result);

        if (result == JudgeResult::FAIL)
        {
            // Keep the best record, but reset the current run score.
            if (currentScore > highScore) highScore = currentScore;
            currentScore = 0;
            lastScore = 0;
            return;
        }

        currentScore += lastScore;
        if (currentScore > highScore) highScore = currentScore;
    }

    void ResetTotal()
    {
        currentScore = 0;
        lastScore = 0;
    }

    void ResetAll()
    {
        currentScore = 0;
        highScore = 0;
        lastScore = 0;
    }

    void ResetLast()
    {
        lastScore = 0;
    }

    int GetTotalScore() const { return currentScore; }
    int GetCurrentScore() const { return currentScore; }
    int GetHighScore() const { return highScore; }
    int GetLastScore() const { return lastScore; }

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}
    void Update(float dt) override {}
    void Render(GraphicsContext* gfx) override {}
};

class GameStateMachineComponent : public Component
{
private:
    GameState state = GameState::TITLE;
    MapRandomizerComponent* map = nullptr;
    DepthTargetComponent* target = nullptr;
    ProneSlideComponent* player = nullptr;
    FollowCameraComponent* camera = nullptr;
    JudgeComponent* judge = nullptr;
    ScoreComponent* score = nullptr;
    float judgeTimer = 0.0f;
    bool lastSpace = false;
    bool lastR = false;

public:
    GameStateMachineComponent(
        MapRandomizerComponent* mapRandomizer,
        DepthTargetComponent* depthTarget,
        ProneSlideComponent* playerMotion,
        FollowCameraComponent* followCamera,
        JudgeComponent* judgeComp,
        ScoreComponent* scoreComp)
        : map(mapRandomizer), target(depthTarget), player(playerMotion), camera(followCamera), judge(judgeComp), score(scoreComp)
    {
    }

    GameState GetState() const { return state; }

    void ChangeState(GameState next)
    {
        state = next;
        judgeTimer = 0.0f;
        printf("[FSM] State -> %s\n", ToString(state));
    }

    void StartNewRound()
    {
        if (map) map->SelectRandomMap();
        if (target) target->Randomize();
        if (player) player->Reset();
        if (camera) camera->Reset();
        if (judge) judge->Reset();
        if (score) score->ResetLast();
        ChangeState(GameState::READY);

        if (map && target)
        {
            printf("[Round] %s / %s / inertia %.1fs / receiverZ %.1f\n",
                map->CurrentMap().label,
                map->CurrentMap().name,
                map->CurrentMap().inertiaSeconds,
                target->GetTargetZ());
        }
    }

    void EvaluateAndMoveToResult()
    {
        if (!judge || !score) return;
        JudgeResult r = judge->Evaluate();
        score->AddScore(r);
        ChangeState(GameState::JUDGE);

        printf("[Judge] %s / error %.2f / +%d / total %d\n",
            ToString(r), judge->GetError(), score->GetLastScore(), score->GetTotalScore());
    }

    void Start(GraphicsContext* gfx) override {}

    void Input() override
    {
        bool nowSpace = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        bool nowR = (GetAsyncKeyState('R') & 0x8000) != 0;
        bool spacePressed = nowSpace && !lastSpace;
        bool rPressed = nowR && !lastR;
        lastSpace = nowSpace;
        lastR = nowR;

        if (rPressed)
        {
            StartNewRound();
            return;
        }

        if (!spacePressed) return;

        if (state == GameState::TITLE)
        {
            StartNewRound();
        }
        else if (state == GameState::READY)
        {
            if (player) player->StartApproach();
            ChangeState(GameState::APPROACH);
        }
        else if (state == GameState::APPROACH)
        {
            float inertia = map ? map->GetInertiaSeconds() : 0.0f;
            if (player) player->StartProneSlide(inertia);

            if (inertia <= 0.0f)
            {
                EvaluateAndMoveToResult();
            }
            else
            {
                ChangeState(GameState::PRONE_SLIDE);
            }
        }
        else if (state == GameState::RESULT)
        {
            StartNewRound();
        }
    }

    void Update(float dt) override
    {
        if (state == GameState::PRONE_SLIDE && player && map)
        {
            player->UpdateSlide(dt, map->GetInertiaSeconds());
        }

        if ((state == GameState::APPROACH || state == GameState::PRONE_SLIDE) && player && target)
        {
            if (player->GetWorldZ() >= target->GetTargetZ())
            {
                player->ForceStop();
                EvaluateAndMoveToResult();
                return;
            }
        }

        if (state == GameState::PRONE_SLIDE && player && player->IsSlideFinished())
        {
            EvaluateAndMoveToResult();
            return;
        }

        if (state == GameState::JUDGE)
        {
            judgeTimer += dt;
            if (judgeTimer >= 0.8f)
            {
                ChangeState(GameState::RESULT);
            }
        }
    }

    void Render(GraphicsContext* gfx) override {}
};

class ResultFlashComponent : public Component
{
private:
    GameStateMachineComponent* fsm = nullptr;
    JudgeComponent* judge = nullptr;
    ColorMaterial* material = nullptr;
    GameState lastState = GameState::TITLE;
    float flashTimer = 0.0f;

public:
    ResultFlashComponent(GameStateMachineComponent* stateMachine, JudgeComponent* judgeComp, ColorMaterial* mat)
        : fsm(stateMachine), judge(judgeComp), material(mat) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!fsm || !judge || !material) return;

        GameState state = fsm->GetState();
        if (state == GameState::JUDGE && lastState != GameState::JUDGE)
        {
            flashTimer = 0.42f;
        }
        lastState = state;

        XMFLOAT4 color = { 1, 1, 1, 0 };
        if (flashTimer > 0.0f)
        {
            flashTimer -= dt;
            float t = ClampF(flashTimer / 0.42f, 0.0f, 1.0f);
            float alpha = 0.28f * t * t;

            switch (judge->GetResult())
            {
            case JudgeResult::PERFECT: color = { 1.00f, 0.91f, 0.30f, alpha }; break;
            case JudgeResult::GREAT:   color = { 0.30f, 0.75f, 1.00f, alpha * 0.9f }; break;
            case JudgeResult::GOOD:    color = { 0.35f, 1.00f, 0.45f, alpha * 0.85f }; break;
            case JudgeResult::FAIL:    color = { 1.00f, 0.24f, 0.24f, alpha * 1.05f }; break;
            default: break;
            }
        }

        material->SetColor(color);
        pOwner->pos = { 0.0f, 0.0f, 0.0f };
        pOwner->scale = { 2.10f, 2.10f, 1.0f };
    }

    void Render(GraphicsContext* gfx) override {}
};

class ResultImageOverlayComponent : public Component
{
private:
    GameStateMachineComponent* fsm = nullptr;
    JudgeComponent* judge = nullptr;
    MeshRenderer* renderer = nullptr;
    TextureMaterial* perfectMat = nullptr;
    TextureMaterial* greatMat = nullptr;
    TextureMaterial* goodMat = nullptr;
    TextureMaterial* failMat = nullptr;
    JudgeResult shownResult = JudgeResult::NONE;
    bool visible = false;
    float timer = 0.0f;

public:
    ResultImageOverlayComponent(GameStateMachineComponent* stateMachine, JudgeComponent* judgeComp,
        MeshRenderer* meshRenderer,
        TextureMaterial* perfectMaterial,
        TextureMaterial* greatMaterial,
        TextureMaterial* goodMaterial,
        TextureMaterial* failMaterial)
        : fsm(stateMachine), judge(judgeComp), renderer(meshRenderer),
        perfectMat(perfectMaterial), greatMat(greatMaterial), goodMat(goodMaterial), failMat(failMaterial)
    {
    }

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    TextureMaterial* SelectMaterial(JudgeResult r)
    {
        switch (r)
        {
        case JudgeResult::PERFECT: return perfectMat;
        case JudgeResult::GREAT:   return greatMat;
        case JudgeResult::GOOD:    return goodMat;
        case JudgeResult::FAIL:    return failMat;
        default: return nullptr;
        }
    }

    void Update(float dt) override
    {
        if (!fsm || !judge || !renderer)
        {
            pOwner->scale = { 0.0f, 0.0f, 1.0f };
            return;
        }

        GameState state = fsm->GetState();
        JudgeResult current = judge->GetResult();
        bool shouldShow = (state == GameState::JUDGE || state == GameState::RESULT) && current != JudgeResult::NONE;

        if (shouldShow)
        {
            if (!visible || current != shownResult)
            {
                timer = 0.0f;
                visible = true;
                shownResult = current;
                renderer->pMaterial = SelectMaterial(current);
            }

            timer += dt;
            float pop = 1.0f - expf(-7.0f * timer);
            float bounce = sinf(timer * 18.0f) * 0.18f * expf(-5.0f * timer);
            float pulse = 1.0f + 0.03f * sinf(timer * 6.0f);
            float scale = (0.88f * pop + bounce) * pulse;
            if (scale < 0.0f) scale = 0.0f;

            pOwner->pos = { 0.0f, 0.18f + 0.02f * sinf(timer * 5.0f) * expf(-2.6f * timer), 0.0f };
            pOwner->rot = { 0.0f, 0.0f, 0.035f * sinf(timer * 7.0f) * expf(-2.8f * timer) };
            pOwner->scale = { 1.55f * scale, 0.52f * scale, 1.0f };
        }
        else
        {
            visible = false;
            shownResult = JudgeResult::NONE;
            timer = 0.0f;
            pOwner->scale = { 0.0f, 0.0f, 1.0f };
        }
    }

    void Render(GraphicsContext* gfx) override {}
};



class ResultCameraImpactComponent : public Component
{
private:
    GameStateMachineComponent* fsm = nullptr;
    JudgeComponent* judge = nullptr;
    FollowCameraComponent* camera = nullptr;
    GameState lastState = GameState::TITLE;

public:
    ResultCameraImpactComponent(GameStateMachineComponent* stateMachine, JudgeComponent* judgeComp, FollowCameraComponent* cam)
        : fsm(stateMachine), judge(judgeComp), camera(cam) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!fsm || !judge || !camera) return;

        GameState state = fsm->GetState();
        if (state == GameState::JUDGE && lastState != GameState::JUDGE)
        {
            float impact = 0.65f;
            switch (judge->GetResult())
            {
            case JudgeResult::PERFECT: impact = 1.05f; break;
            case JudgeResult::GREAT:   impact = 0.82f; break;
            case JudgeResult::GOOD:    impact = 0.62f; break;
            case JudgeResult::FAIL:    impact = 1.55f; break;
            default: break;
            }
            camera->TriggerImpactShake(impact);
        }

        lastState = state;
    }

    void Render(GraphicsContext* gfx) override {}
};

class ReceiverResultAuraComponent : public Component
{
private:
    BackgroundZoomComponent* background = nullptr;
    DepthTargetComponent* target = nullptr;
    JudgeComponent* judge = nullptr;
    GameStateMachineComponent* fsm = nullptr;
    ColorMaterial* material = nullptr;
    float timer = 0.0f;

public:
    ReceiverResultAuraComponent(BackgroundZoomComponent* bg, DepthTargetComponent* depthTarget,
        JudgeComponent* judgeComp, GameStateMachineComponent* stateMachine, ColorMaterial* mat)
        : background(bg), target(depthTarget), judge(judgeComp), fsm(stateMachine), material(mat) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!background || !target || !judge || !fsm || !material) return;

        GameState state = fsm->GetState();
        JudgeResult result = judge->GetResult();

        if (!((state == GameState::JUDGE || state == GameState::RESULT) && result != JudgeResult::NONE))
        {
            material->SetColor({ 0, 0, 0, 0 });
            pOwner->scale = { 0, 0, 1 };
            timer = 0.0f;
            return;
        }

        timer += dt;

        ProjectedPoint p = background->ProjectTrackZ(target->GetTargetZ());
        if (!p.visible)
        {
            pOwner->scale = { 0, 0, 1 };
            return;
        }

        float pulse = 0.5f + 0.5f * sinf(timer * 9.0f);
        XMFLOAT4 color = { 1, 1, 1, 0.20f };

        switch (result)
        {
        case JudgeResult::PERFECT:
            color = { 1.00f, 0.82f, 0.12f, 0.32f + 0.13f * pulse };
            break;
        case JudgeResult::GREAT:
            color = { 0.28f, 0.72f, 1.00f, 0.24f + 0.10f * pulse };
            break;
        case JudgeResult::GOOD:
            color = { 0.36f, 1.00f, 0.42f, 0.20f + 0.08f * pulse };
            break;
        case JudgeResult::FAIL:
            color = { 1.00f, 0.10f, 0.08f, 0.34f + 0.18f * pulse };
            break;
        default:
            break;
        }

        material->SetColor(color);

        float size = (result == JudgeResult::FAIL ? 0.42f : 0.36f) * p.scale * (1.0f + 0.12f * pulse);
        pOwner->pos = { p.x, p.y + size * 0.48f, 0.0f };
        pOwner->scale = { size, size * 1.10f, 1.0f };
    }

    void Render(GraphicsContext* gfx) override {}
};

class ResultHaloBurstComponent : public Component
{
private:
    GameStateMachineComponent* fsm = nullptr;
    JudgeComponent* judge = nullptr;
    MeshRenderer* renderer = nullptr;
    TextureMaterial* haloMaterial = nullptr;
    GameState lastState = GameState::TITLE;
    float timer = 0.0f;
    bool activeBurst = false;
    float baseScale = 1.0f;
    float speed = 1.0f;
    float rotationSpeed = 0.0f;

public:
    ResultHaloBurstComponent(GameStateMachineComponent* stateMachine, JudgeComponent* judgeComp,
        MeshRenderer* meshRenderer, TextureMaterial* mat,
        float base, float spd, float rot)
        : fsm(stateMachine), judge(judgeComp), renderer(meshRenderer), haloMaterial(mat),
        baseScale(base), speed(spd), rotationSpeed(rot) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!fsm || !judge || !renderer || !haloMaterial)
        {
            pOwner->scale = { 0, 0, 1 };
            return;
        }

        GameState state = fsm->GetState();
        JudgeResult result = judge->GetResult();

        if (state == GameState::JUDGE && lastState != GameState::JUDGE)
        {
            activeBurst = (result == JudgeResult::PERFECT || result == JudgeResult::GREAT || result == JudgeResult::GOOD);
            timer = 0.0f;
        }
        lastState = state;

        if (!(state == GameState::JUDGE || state == GameState::RESULT) || !activeBurst)
        {
            pOwner->scale = { 0, 0, 1 };
            return;
        }

        timer += dt;
        renderer->pMaterial = haloMaterial;

        float grow = 1.0f - expf(-speed * timer);
        float pulse = 1.0f + 0.06f * sinf(timer * 8.0f);
        float size = baseScale * (0.65f + grow * 0.55f) * pulse;

        pOwner->pos = { 0.0f, 0.14f, 0.0f };
        pOwner->rot = { 0.0f, 0.0f, timer * rotationSpeed };
        pOwner->scale = { size, size, 1.0f };
    }

    void Render(GraphicsContext* gfx) override {}
};

class FailureSlashComponent : public Component
{
private:
    GameStateMachineComponent* fsm = nullptr;
    JudgeComponent* judge = nullptr;
    ColorMaterial* material = nullptr;
    GameState lastState = GameState::TITLE;
    float timer = 0.0f;
    bool active = false;
    float angle = 0.0f;
    float y = 0.0f;

public:
    FailureSlashComponent(GameStateMachineComponent* stateMachine, JudgeComponent* judgeComp, ColorMaterial* mat, float rot, float ypos)
        : fsm(stateMachine), judge(judgeComp), material(mat), angle(rot), y(ypos) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!fsm || !judge || !material) return;

        GameState state = fsm->GetState();
        if (state == GameState::JUDGE && lastState != GameState::JUDGE)
        {
            active = (judge->GetResult() == JudgeResult::FAIL);
            timer = 0.0f;
        }
        lastState = state;

        if (!(state == GameState::JUDGE || state == GameState::RESULT) || !active)
        {
            pOwner->scale = { 0, 0, 1 };
            material->SetColor({ 1, 0, 0, 0 });
            return;
        }

        timer += dt;
        float alpha = 0.0f;
        if (timer < 0.18f) alpha = timer / 0.18f;
        else alpha = expf(-(timer - 0.18f) * 1.8f);

        material->SetColor({ 1.0f, 0.06f, 0.04f, 0.34f * alpha });
        pOwner->pos = { 0.0f, y, 0.0f };
        pOwner->rot = { 0.0f, 0.0f, angle };
        pOwner->scale = { 2.20f, 0.08f, 1.0f };
    }

    void Render(GraphicsContext* gfx) override {}
};

class ConsoleHudComponent : public Component
{
private:
    GameStateMachineComponent* fsm = nullptr;
    MapRandomizerComponent* map = nullptr;
    ProneSlideComponent* player = nullptr;
    DepthTargetComponent* target = nullptr;
    JudgeComponent* judge = nullptr;
    ScoreComponent* score = nullptr;
    FollowCameraComponent* camera = nullptr;
    float printTimer = 0.0f;

public:
    ConsoleHudComponent(GameStateMachineComponent* fsmComp,
        MapRandomizerComponent* mapComp,
        ProneSlideComponent* playerComp,
        DepthTargetComponent* targetComp,
        JudgeComponent* judgeComp,
        ScoreComponent* scoreComp,
        FollowCameraComponent* cameraComp)
        : fsm(fsmComp), map(mapComp), player(playerComp), target(targetComp), judge(judgeComp), score(scoreComp), camera(cameraComp) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        printTimer += dt;
        if (printTimer < 0.25f) return;
        printTimer = 0.0f;

        if (!fsm || !map || !player || !target || !judge || !score || !camera) return;

        char title[256];
        sprintf_s(title, "Dogeza Sliding | %s | %s %.1fs | Z %.0f -> %.0f | %s | Score %d | Best %d",
            ToString(fsm->GetState()),
            map->CurrentMap().label,
            map->GetInertiaSeconds(),
            player->GetWorldZ(),
            target->GetTargetZ(),
            ToString(judge->GetResult()),
            score->GetCurrentScore(),
            score->GetHighScore());
        SetConsoleTitleA(title);

        printf("\r[%-12s] %-5s inertia:%3.1fs cameraZ:%6.1f playerZ:%6.1f receiverZ:%6.1f vel:%6.1f result:%-7s score:%4d best:%4d     ",
            ToString(fsm->GetState()),
            map->CurrentMap().label,
            map->GetInertiaSeconds(),
            camera->GetCameraZ(),
            player->GetWorldZ(),
            target->GetTargetZ(),
            player->GetVelocity(),
            ToString(judge->GetResult()),
            score->GetTotalScore(),
            score->GetHighScore());
    }

    void Render(GraphicsContext* gfx) override {}
};



class TitleOverlayComponent : public Component
{
private:
    GameStateMachineComponent* fsm = nullptr;
    float timer = 0.0f;
    float baseX = 0.0f;
    float baseY = 0.16f;
    float baseScaleX = 1.52f;
    float baseScaleY = 0.56f;

public:
    TitleOverlayComponent(GameStateMachineComponent* stateMachine,
        float x = 0.0f, float y = 0.16f, float sx = 1.52f, float sy = 0.56f)
        : fsm(stateMachine), baseX(x), baseY(y), baseScaleX(sx), baseScaleY(sy) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!fsm || fsm->GetState() != GameState::TITLE)
        {
            timer = 0.0f;
            pOwner->scale = { 0.0f, 0.0f, 1.0f };
            return;
        }

        timer += dt;
        float pulse = 1.0f + 0.025f * sinf(timer * 2.6f);
        float bob = 0.018f * sinf(timer * 1.7f);
        float tilt = 0.015f * sinf(timer * 1.15f);

        pOwner->pos = { baseX, baseY + bob, 0.0f };
        pOwner->rot = { 0.0f, 0.0f, tilt };
        pOwner->scale = { baseScaleX * pulse, baseScaleY * pulse, 1.0f };
    }

    void Render(GraphicsContext* gfx) override {}
};

class TitleBackdropComponent : public Component
{
private:
    GameStateMachineComponent* fsm = nullptr;
    ColorMaterial* material = nullptr;

public:
    TitleBackdropComponent(GameStateMachineComponent* stateMachine, ColorMaterial* mat)
        : fsm(stateMachine), material(mat) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}

    void Update(float dt) override
    {
        if (!fsm || !material) return;

        if (fsm->GetState() == GameState::TITLE)
        {
            material->SetColor({ 0.02f, 0.02f, 0.03f, 0.56f });
            pOwner->pos = { 0.0f, 0.0f, 0.0f };
            pOwner->scale = { 2.10f, 2.10f, 1.0f };
        }
        else
        {
            material->SetColor({ 0, 0, 0, 0 });
            pOwner->scale = { 0.0f, 0.0f, 1.0f };
        }
    }

    void Render(GraphicsContext* gfx) override {}
};

class BitmapFontHudComponent : public Component
{
private:
    TextureMaterial* fontMaterial = nullptr;
    GameStateMachineComponent* fsm = nullptr;
    MapRandomizerComponent* map = nullptr;
    ProneSlideComponent* player = nullptr;
    DepthTargetComponent* target = nullptr;
    JudgeComponent* judge = nullptr;
    ScoreComponent* score = nullptr;

    ID3D11Buffer* cBuffer = nullptr;
    ID3D11Buffer* vBuffer = nullptr;
    UINT vertexCount = 0;
    std::string lastKey;
    float blinkTimer = 0.0f;
    const std::string charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .:-+/=_|!";
    int columns = 16;

public:
    BitmapFontHudComponent(TextureMaterial* mat,
        GameStateMachineComponent* fsmComp,
        MapRandomizerComponent* mapComp,
        ProneSlideComponent* playerComp,
        DepthTargetComponent* targetComp,
        JudgeComponent* judgeComp,
        ScoreComponent* scoreComp)
        : fontMaterial(mat), fsm(fsmComp), map(mapComp), player(playerComp), target(targetComp), judge(judgeComp), score(scoreComp)
    {
    }

    ~BitmapFontHudComponent()
    {
        if (cBuffer) cBuffer->Release();
        if (vBuffer) vBuffer->Release();
    }

    void Start(GraphicsContext* gfx) override
    {
        D3D11_BUFFER_DESC cbd = {};
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ConstantBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        gfx->Device->CreateBuffer(&cbd, nullptr, &cBuffer);
    }

    void Input() override {}
    void Update(float dt) override { blinkTimer += dt; }

    char NormalizeChar(char c) const
    {
        if (c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
        return c;
    }

    void AddText(std::vector<Vertex>& out, float x, float y, float cw, float ch, const std::string& text) const
    {
        const float rows = (float)((int)(charset.size() + columns - 1) / columns);
        for (size_t i = 0; i < text.size(); ++i)
        {
            char c = NormalizeChar(text[i]);
            size_t idx = charset.find(c);
            if (idx == std::string::npos) idx = charset.find(' ');
            int col = (int)(idx % columns);
            int row = (int)(idx / columns);

            float u0 = (float)col / (float)columns;
            float v0 = (float)row / rows;
            float u1 = (float)(col + 1) / (float)columns;
            float v1 = (float)(row + 1) / rows;

            float x0 = x + (float)i * cw;
            float x1 = x0 + cw;
            float y0 = y;
            float y1 = y - ch;

            out.push_back({ { x0, y0, 0.0f }, { u0, v0, 0.0f, 1.0f } });
            out.push_back({ { x1, y0, 0.0f }, { u1, v0, 0.0f, 1.0f } });
            out.push_back({ { x0, y1, 0.0f }, { u0, v1, 0.0f, 1.0f } });

            out.push_back({ { x1, y1, 0.0f }, { u1, v1, 0.0f, 1.0f } });
            out.push_back({ { x0, y1, 0.0f }, { u0, v1, 0.0f, 1.0f } });
            out.push_back({ { x1, y0, 0.0f }, { u1, v0, 0.0f, 1.0f } });
        }
    }

    void AddCenteredText(std::vector<Vertex>& out, float centerX, float y, float cw, float ch, const std::string& text) const
    {
        float x = centerX - (float)text.size() * cw * 0.5f;
        AddText(out, x, y, cw, ch, text);
    }

    std::string BuildKey() const
    {
        char buf[1024];
        const char* stateText = fsm ? ToString(fsm->GetState()) : "NONE";
        const char* mapLabel = map ? map->CurrentMap().label : "MAP";
        const char* mapName = map ? map->CurrentMap().name : "UNKNOWN";
        float inertia = map ? map->GetInertiaSeconds() : 0.0f;
        float pz = player ? player->GetWorldZ() : 0.0f;
        float tz = target ? target->GetTargetZ() : 0.0f;
        int total = score ? score->GetCurrentScore() : 0;
        int high = score ? score->GetHighScore() : 0;
        int last = score ? score->GetLastScore() : 0;
        JudgeResult jr = judge ? judge->GetResult() : JudgeResult::NONE;
        float dist = tz - pz;
        const char* distState = "TOO FAR";
        if (dist <= 0.0f) distState = "TOO LATE";
        else if (dist <= 25.0f) distState = "PERFECT CHANCE";
        else if (dist <= 60.0f) distState = "GREAT RANGE";
        else if (dist <= 120.0f) distState = "GOOD RANGE";

        sprintf_s(buf,
            "STATE:%s\nMAP:%s\nMAPNAME:%s\nINERTIA:%.1f\nDIST:%.0f\nDISTSTATE:%s\nSCORE:%d\nBEST:%d\nLAST:%d\nRESULT:%s",
            stateText, mapLabel, mapName, inertia, dist, distState, total, high, last, ToString(jr));
        return std::string(buf);
    }

    void Rebuild(GraphicsContext* gfx, const std::string& key)
    {
        if (vBuffer) { vBuffer->Release(); vBuffer = nullptr; }

        std::vector<Vertex> verts;

        std::vector<std::string> lines;
        size_t startPos = 0;
        while (startPos <= key.size())
        {
            size_t endPos = key.find('\n', startPos);
            lines.push_back((endPos == std::string::npos) ? key.substr(startPos) : key.substr(startPos, endPos - startPos));
            if (endPos == std::string::npos) break;
            startPos = endPos + 1;
        }

        auto valueOf = [&](const char* prefix)->std::string {
            for (auto& s : lines)
            {
                std::string p = std::string(prefix);
                if (s.rfind(p, 0) == 0) return s.substr(p.size());
            }
            return std::string();
        };

        std::string mapLabel = valueOf("MAP:");
        std::string mapName = valueOf("MAPNAME:");
        std::string inertia = valueOf("INERTIA:");
        std::string dist = valueOf("DIST:");
        std::string distState = valueOf("DISTSTATE:");
        std::string scoreText = valueOf("SCORE:");
        std::string bestText = valueOf("BEST:");
        std::string lastText = valueOf("LAST:");
        std::string resultText = valueOf("RESULT:");

        bool titleMode = fsm && fsm->GetState() == GameState::TITLE;

        if (!titleMode)
        {
            AddText(verts, -0.94f, 0.93f, 0.030f, 0.054f, std::string("MAP ") + mapLabel + std::string("  ") + mapName);
            AddText(verts, -0.94f, 0.86f, 0.026f, 0.048f, std::string("INERTIA ") + inertia + "S");

            // Keep the score panel farther left and use compact glyph size so numbers never leave the screen.
            AddText(verts, 0.34f, 0.94f, 0.023f, 0.043f, std::string("SCORE ") + scoreText);
            AddText(verts, 0.34f, 0.885f, 0.023f, 0.043f, std::string("BEST ") + bestText);
            AddText(verts, 0.34f, 0.83f, 0.021f, 0.039f, std::string("LAST +") + lastText);
            AddCenteredText(verts, 0.0f, -0.79f, 0.028f, 0.050f, std::string("DIST ") + dist + "  " + distState);
        }

        std::string prompt;
        if (fsm)
        {
            switch (fsm->GetState())
            {
            case GameState::TITLE:
                AddCenteredText(verts, 0.0f, -0.20f, 0.028f, 0.048f, "STOP AS CLOSE AS POSSIBLE");
                AddCenteredText(verts, 0.0f, -0.28f, 0.028f, 0.048f, "DO NOT HIT THE RECEIVER");
                if (sinf(blinkTimer * 3.5f) > 0.0f)
                    AddCenteredText(verts, 0.0f, -0.43f, 0.026f, 0.044f, "PRESS SPACE TO CONTINUE");
                break;
            case GameState::READY:
                AddCenteredText(verts, 0.0f, 0.18f, 0.046f, 0.080f, std::string("ROUND ") + mapLabel);
                AddCenteredText(verts, 0.0f, 0.08f, 0.030f, 0.052f, std::string("INERTIA ") + inertia + "S");
                prompt = "PRESS SPACE TO RUN";
                break;
            case GameState::APPROACH:
                AddCenteredText(verts, 0.0f, 0.12f, 0.034f, 0.058f, "PRESS SPACE TO DOGEZA");
                prompt = "RUNNING";
                break;
            case GameState::PRONE_SLIDE:
                AddCenteredText(verts, 0.0f, 0.12f, 0.032f, 0.056f, "SLIDING");
                prompt = "WAIT FOR THE STOP";
                break;
            case GameState::JUDGE:
                AddCenteredText(verts, 0.0f, -0.02f, 0.026f, 0.046f, std::string("RESULT ") + resultText);
                prompt = "JUDGING";
                break;
            case GameState::RESULT:
                AddCenteredText(verts, 0.0f, -0.02f, 0.026f, 0.046f, std::string("RESULT ") + resultText);
                if (resultText == "FAIL")
                    AddCenteredText(verts, 0.0f, -0.10f, 0.026f, 0.046f, "SCORE RESET  BEST KEPT");
                else
                    AddCenteredText(verts, 0.0f, -0.10f, 0.026f, 0.046f, std::string("CURRENT ") + scoreText + "  BEST " + bestText);
                AddCenteredText(verts, 0.0f, -0.18f, 0.028f, 0.048f, "SPACE OR R FOR NEXT ROUND");
                prompt = "READY FOR NEXT ROUND";
                break;
            }
        }

        if (!titleMode)
            AddCenteredText(verts, 0.0f, -0.89f, 0.026f, 0.046f, prompt);

        vertexCount = (UINT)verts.size();
        if (vertexCount == 0) return;

        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(Vertex) * vertexCount;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = verts.data();
        gfx->Device->CreateBuffer(&bd, &sd, &vBuffer);
    }

    void Render(GraphicsContext* gfx) override
    {
        if (!fontMaterial || !gfx) return;

        std::string key = BuildKey();
        bool titleMode = fsm && fsm->GetState() == GameState::TITLE;
        if (key != lastKey || !vBuffer || titleMode)
        {
            Rebuild(gfx, key);
            lastKey = key;
        }

        if (!vBuffer || vertexCount == 0) return;

        fontMaterial->Bind(gfx->ImmediateContext);

        ConstantBuffer cb;
        cb.matWorld = XMMatrixTranspose(XMMatrixIdentity());
        gfx->ImmediateContext->UpdateSubresource(cBuffer, 0, nullptr, &cb, 0, 0);
        gfx->ImmediateContext->VSSetConstantBuffers(0, 1, &cBuffer);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        gfx->ImmediateContext->IASetVertexBuffers(0, 1, &vBuffer, &stride, &offset);
        gfx->ImmediateContext->Draw(vertexCount, 0);
    }
};
