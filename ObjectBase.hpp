#pragma once
#include "Framework.hpp"
#include "GraphicsContext.hpp"

class GameObject;

class Component {
public:
    GameObject* pOwner = nullptr;
    bool isStarted = false;

    virtual void Start(GraphicsContext* gfx) = 0;
    virtual void Input() = 0;
    virtual void Update(float dt) = 0;
    virtual void Render(GraphicsContext* gfx) = 0;
    virtual ~Component() {}
};

class GameObject {
public:
    std::string name;
    bool active = true;
    XMFLOAT3 pos = { 0, 0, 0 };
    XMFLOAT3 rot = { 0, 0, 0 };
    XMFLOAT3 scale = { 1, 1, 1 };
    std::vector<Component*> components;

    GameObject(float x, float y, float z, const std::string& objName = "GameObject")
        : name(objName) {
        pos = { x, y, z };
    }

    ~GameObject() {
        for (auto c : components) delete c;
    }

    void SetActive(bool value) { active = value; }

    void AddComponent(Component* c) {
        c->pOwner = this;
        components.push_back(c);
    }

    template<typename T>
    T* GetComponent() {
        for (auto c : components) {
            T* result = dynamic_cast<T*>(c);
            if (result) return result;
        }
        return nullptr;
    }

    void Input() {
        if (!active) return;
        for (auto c : components) c->Input();
    }

    void Update(float dt, GraphicsContext* gfx) {
        if (!active) return;
        for (auto c : components) {
            if (!c->isStarted) {
                c->Start(gfx);
                c->isStarted = true;
            }
            c->Update(dt);
        }
    }

    void Render(GraphicsContext* gfx) {
        if (!active) return;
        for (auto c : components) c->Render(gfx);
    }
};
