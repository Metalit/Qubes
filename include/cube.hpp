#pragma once

#include "modconfig.hpp"

#include "custom-types/shared/coroutine.hpp"

#include "GlobalNamespace/BoxCuttableBySaber.hpp"
#include "GlobalNamespace/Saber.hpp"
#include "GlobalNamespace/VRController.hpp"
#include "GlobalNamespace/ColorPickerButtonController.hpp"

#include "UnityEngine/Material.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"

namespace QuestUI{ class IncrementSetting; }

DECLARE_CLASS_CODEGEN(Qubes, EditMenu, UnityEngine::MonoBehaviour,
    DECLARE_INSTANCE_METHOD(void, LateUpdate);

    public:

    void init(Qubes::DefaultCube* cube);
    QuestUI::IncrementSetting *typeInc, *eventInc;
    UnityEngine::UI::VerticalLayoutGroup *valVertical, *colVertical;
    UnityEngine::UI::Button *closeButton, *lockButton;
    GlobalNamespace::ColorPickerButtonController* colButtonController;
    
    DECLARE_SIMPLE_DTOR();
)

DECLARE_CLASS_CODEGEN(Qubes, DefaultCube, UnityEngine::MonoBehaviour,
    public:

    void init(UnityEngine::Color color, int cubeType, int onHit, float cubeSize, bool locked, Qubes::QubesConfig& config, int index);

    UnityEngine::Transform* findTransform(std::string_view name);
    
    void setColor(UnityEngine::Color color);
    void setType(int cubeType);
    void setSize(float size);

    void setHitAction(int action) { hitAction = action; }
    void setLocked(bool lock) { locked = lock; }
    UnityEngine::Color getColor() { return color; }
    int getType() { return type; }
    int getHitAction() { return hitAction; }
    float getSize() { return size; }
    bool getLocked() { return locked; }

    void setActive(bool active);
    void setMenuActive(bool active);

    void save();

    int index; // for editing in config

    protected:

    void makeMenu();
    
    bool typeSet;

    UnityEngine::Material* material;

    Qubes::QubesConfig* config; // idk about reference variables in classes, it caused complaints

    Qubes::EditMenu* menu;
    bool menuActive;

    UnityEngine::Color color;
    int type;
    int hitAction;
    float size;
    bool locked;
)

// a default cube, but cuttable and interactible
DECLARE_CLASS_CUSTOM(Qubes, Cube, Qubes::DefaultCube,
    DECLARE_INSTANCE_METHOD(void, Update);
    DECLARE_INSTANCE_METHOD(void, LateUpdate);

    public:

    void init(UnityEngine::Color color, int cubeType, int onHit, float cubeSize, bool locked, Qubes::QubesConfig& config, int index);

    void handleCut(GlobalNamespace::Saber* saber, UnityEngine::Vector3 cutPoint, UnityEngine::Quaternion orientation, UnityEngine::Vector3 cutDirVec);
    void setCuttableDelay(bool cuttable, float seconds);
    
    void setMenuActive(bool active);
    bool deletePressed(UnityEngine::Transform* hit);
    void editPressed(UnityEngine::Transform* hit);
    
    private:

    custom_types::Helpers::Coroutine cuttableCoroutine(bool cuttable, float seconds);
    custom_types::Helpers::Coroutine respawnCoroutine();

    GlobalNamespace::BoxCuttableBySaber* hitbox;

    GlobalNamespace::VRController* controller;
    UnityEngine::Vector3 grabPos;
    UnityEngine::Quaternion grabRot;
)