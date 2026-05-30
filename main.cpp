#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

#include "GameLoop.hpp"
#include "MeshRenderer.hpp"
#include "PlayerControl.hpp"
#include "DogezaComponents.hpp"

LRESULT CALLBACK GlobalWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(h, m, w, l);
}

Mesh* CreateColorQuadMesh(GraphicsContext* gfx)
{
    std::vector<Vertex> v;
    XMFLOAT4 white = { 1, 1, 1, 1 };

    v.push_back({ {-0.5f,  0.5f, 0.0f}, white });
    v.push_back({ { 0.5f,  0.5f, 0.0f}, white });
    v.push_back({ {-0.5f, -0.5f, 0.0f}, white });

    v.push_back({ { 0.5f, -0.5f, 0.0f}, white });
    v.push_back({ {-0.5f, -0.5f, 0.0f}, white });
    v.push_back({ { 0.5f,  0.5f, 0.0f}, white });

    Mesh* mesh = new Mesh();
    mesh->Create(gfx, v);
    return mesh;
}

Mesh* CreateTextureQuadMesh(GraphicsContext* gfx)
{
    std::vector<Vertex> v;

    v.push_back({ {-0.5f,  0.5f, 0.0f}, {0, 0, 0, 1} });
    v.push_back({ { 0.5f,  0.5f, 0.0f}, {1, 0, 0, 1} });
    v.push_back({ {-0.5f, -0.5f, 0.0f}, {0, 1, 0, 1} });

    v.push_back({ { 0.5f, -0.5f, 0.0f}, {1, 1, 0, 1} });
    v.push_back({ {-0.5f, -0.5f, 0.0f}, {0, 1, 0, 1} });
    v.push_back({ { 0.5f,  0.5f, 0.0f}, {1, 0, 0, 1} });

    Mesh* mesh = new Mesh();
    mesh->Create(gfx, v);
    return mesh;
}

GameObject* MakeRenderObject(const std::string& name, Mesh* mesh, Material* material)
{
    GameObject* obj = new GameObject(0.0f, 0.0f, 0.0f, name);
    obj->AddComponent(new MeshRenderer(mesh, material));
    return obj;
}

Texture* LoadTextureAsset(GraphicsContext* gfx, const wchar_t* path)
{
    Texture* tex = new Texture();
    if (!tex->Load(gfx->Device, path))
    {
        char msg[256];
        sprintf_s(msg, "Texture load failed. Check file path: %ls", path);
        MessageBoxA(nullptr, msg, "Texture Error", MB_ICONERROR);
    }
    tex->CreateSampler(gfx->Device);
    return tex;
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int)
{
    srand((unsigned int)time(nullptr));

    printf("=== Dogeza Sliding: Simplified Core Version ===\n");
    printf("Space: start / prone slide, R: restart, ESC: quit\n");
    printf("Core version: map rule, prone slide, judge, score, camera/background, title, HUD, and result image.\n\n");

    GameLoop gEngine;
    gEngine.Initialize(hI, GlobalWndProc);

    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    ShaderSet colorShaders = gEngine.gfx.CompileAndCreate(L"effect.hlsl", 0, true, ied, ARRAYSIZE(ied));
    ShaderSet textureShaders = gEngine.gfx.CompileAndCreate(L"texture.hlsl", 0, true, ied, ARRAYSIZE(ied));

    Mesh* colorQuadMesh = CreateColorQuadMesh(&gEngine.gfx);
    Mesh* textureQuadMesh = CreateTextureQuadMesh(&gEngine.gfx);

    Texture* backgroundTex = LoadTextureAsset(&gEngine.gfx, L"assets\\bg_map_stop.png");
    Texture* backgroundShortTex = LoadTextureAsset(&gEngine.gfx, L"assets\\bg_map_short.png");
    Texture* backgroundLongTex = LoadTextureAsset(&gEngine.gfx, L"assets\\bg_map_long.png");
    Texture* playerRunTex = LoadTextureAsset(&gEngine.gfx, L"assets\\player_run.png");
    Texture* playerProneTex = LoadTextureAsset(&gEngine.gfx, L"assets\\player_prone.png");
    Texture* receiverTex = LoadTextureAsset(&gEngine.gfx, L"assets\\receiver.png");
    Texture* fontTex = LoadTextureAsset(&gEngine.gfx, L"assets\\font_atlas.png");
    Texture* resultPerfectTex = LoadTextureAsset(&gEngine.gfx, L"assets\\result_perfect.png");
    Texture* resultGreatTex = LoadTextureAsset(&gEngine.gfx, L"assets\\result_great.png");
    Texture* resultGoodTex = LoadTextureAsset(&gEngine.gfx, L"assets\\result_good.png");
    Texture* resultFailTex = LoadTextureAsset(&gEngine.gfx, L"assets\\result_fail.png");
    Texture* titleCardTex = LoadTextureAsset(&gEngine.gfx, L"assets\\title_card.png");

    ColorMaterial* topBarMat = new ColorMaterial(colorShaders, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.34f), gEngine.gfx.Device);
    ColorMaterial* bottomBarMat = new ColorMaterial(colorShaders, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.36f), gEngine.gfx.Device);
    ColorMaterial* titleBackdropMat = new ColorMaterial(colorShaders, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), gEngine.gfx.Device);

    TextureMaterial* backgroundMat = new TextureMaterial(textureShaders, backgroundTex);
    TextureMaterial* playerRunMat = new TextureMaterial(textureShaders, playerRunTex);
    TextureMaterial* playerProneMat = new TextureMaterial(textureShaders, playerProneTex);
    TextureMaterial* receiverTexMat = new TextureMaterial(textureShaders, receiverTex);
    TextureMaterial* fontMat = new TextureMaterial(textureShaders, fontTex);
    TextureMaterial* resultPerfectMat = new TextureMaterial(textureShaders, resultPerfectTex);
    TextureMaterial* resultGreatMat = new TextureMaterial(textureShaders, resultGreatTex);
    TextureMaterial* resultGoodMat = new TextureMaterial(textureShaders, resultGoodTex);
    TextureMaterial* resultFailMat = new TextureMaterial(textureShaders, resultFailTex);
    TextureMaterial* titleCardMat = new TextureMaterial(textureShaders, titleCardTex);

    std::vector<Mesh*> meshes = { colorQuadMesh, textureQuadMesh };
    std::vector<Material*> materials = {
        topBarMat, bottomBarMat, titleBackdropMat,
        backgroundMat, playerRunMat, playerProneMat, receiverTexMat, fontMat,
        resultPerfectMat, resultGreatMat, resultGoodMat, resultFailMat, titleCardMat
    };
    std::vector<Texture*> textures = {
        backgroundTex, backgroundShortTex, backgroundLongTex,
        playerRunTex, playerProneTex, receiverTex, fontTex,
        resultPerfectTex, resultGreatTex, resultGoodTex, resultFailTex, titleCardTex
    };

    GameObject* manager = new GameObject(0.0f, 0.0f, 0.0f, "GameManagerObject");

    auto* mapRandomizer = new MapRandomizerComponent();
    auto* depthTarget = new DepthTargetComponent();
    auto* playerMotion = new ProneSlideComponent();
    auto* followCamera = new FollowCameraComponent(playerMotion);
    auto* judge = new JudgeComponent(playerMotion, depthTarget);
    auto* score = new ScoreComponent();

    auto* fsm = new GameStateMachineComponent(
        mapRandomizer,
        depthTarget,
        playerMotion,
        followCamera,
        judge,
        score);

    manager->AddComponent(mapRandomizer);
    manager->AddComponent(depthTarget);
    manager->AddComponent(playerMotion);
    manager->AddComponent(judge);
    manager->AddComponent(score);
    manager->AddComponent(fsm);
    manager->AddComponent(followCamera);

    GameObject* background = new GameObject(0.0f, 0.0f, 0.0f, "SchoolBackgroundZoom");
    auto* backgroundZoom = new BackgroundZoomComponent(backgroundMat, followCamera, mapRandomizer, backgroundTex, backgroundShortTex, backgroundLongTex);
    background->AddComponent(backgroundZoom);
    gEngine.world.push_back(background);
    gEngine.world.push_back(manager);

    GameObject* receiver = MakeRenderObject("DogezaReceiverNPC", textureQuadMesh, receiverTexMat);
    receiver->AddComponent(new ReceiverVisualComponent(backgroundZoom, depthTarget));
    gEngine.world.push_back(receiver);

    GameObject* player = MakeRenderObject("PlayerDogezaSlider", textureQuadMesh, playerRunMat);
    MeshRenderer* playerRenderer = player->GetComponent<MeshRenderer>();
    player->AddComponent(new PlayerVisualComponent(followCamera, playerMotion, playerRenderer, playerRunMat, playerProneMat));
    gEngine.world.push_back(player);

    GameObject* topBar = MakeRenderObject("TopHudBar", colorQuadMesh, topBarMat);
    topBar->pos = { 0.0f, 0.90f, 0.0f };
    topBar->scale = { 1.95f, 0.22f, 1.0f };
    gEngine.world.push_back(topBar);

    GameObject* bottomBar = MakeRenderObject("BottomHudBar", colorQuadMesh, bottomBarMat);
    bottomBar->pos = { 0.0f, -0.86f, 0.0f };
    bottomBar->scale = { 1.95f, 0.17f, 1.0f };
    gEngine.world.push_back(bottomBar);

    GameObject* titleBackdrop = MakeRenderObject("TitleBackdrop", colorQuadMesh, titleBackdropMat);
    titleBackdrop->AddComponent(new TitleBackdropComponent(fsm, titleBackdropMat));
    gEngine.world.push_back(titleBackdrop);

    GameObject* titleCard = MakeRenderObject("TitleCard", textureQuadMesh, titleCardMat);
    titleCard->AddComponent(new TitleOverlayComponent(fsm, 0.0f, 0.16f, 1.52f, 0.56f));
    gEngine.world.push_back(titleCard);

    GameObject* hud = new GameObject(0.0f, 0.0f, 0.0f, "BitmapFontHUD");
    hud->AddComponent(new BitmapFontHudComponent(fontMat, fsm, mapRandomizer, playerMotion, depthTarget, judge, score));
    gEngine.world.push_back(hud);

    GameObject* resultImage = MakeRenderObject("ResultImageOverlay", textureQuadMesh, resultPerfectMat);
    MeshRenderer* resultRenderer = resultImage->GetComponent<MeshRenderer>();
    resultImage->AddComponent(new ResultImageOverlayComponent(fsm, judge, resultRenderer, resultPerfectMat, resultGreatMat, resultGoodMat, resultFailMat));
    gEngine.world.push_back(resultImage);

    mapRandomizer->SelectRandomMap();
    depthTarget->Randomize();

    gEngine.Run();

    for (Material* m : materials) delete m;
    for (Texture* t : textures) delete t;
    for (Mesh* mesh : meshes) delete mesh;
    colorShaders.Release();
    textureShaders.Release();

    return 0;
}
