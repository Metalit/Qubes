#pragma once

#include "custom-types/shared/coroutine.hpp"

#include "extern/custom-types/shared/macros.hpp"
#include "extern/custom-types/shared/register.hpp"

#include "GlobalNamespace/INoteDebrisDidFinishEvent.hpp"
#include "GlobalNamespace/BoxCuttableBySaber.hpp"
#include "GlobalNamespace/Saber.hpp"

#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Physics.hpp"

#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "GlobalNamespace/ColorPickerButtonController.hpp"

#include "GlobalNamespace/VRController.hpp"

const std::vector<std::string> cubeTypes = { "Blank", "Dot", "Arrow" };
const std::vector<std::string> cutEvents = { "None", "Pause", "Restart", "Menu", "Crash" };

namespace Qubes{ class DefaultCube; }
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

    UnityEngine::Transform* findTransform(std::string_view name);

    void init(UnityEngine::Color color, int cubeType, int onHit, float cubeSize, bool locked, int index);
    void makeMenu();
    
    void setColor(UnityEngine::Color color);
    void setType(int cubeType);
    void setSize(float size);
    
    bool typeSet;

    UnityEngine::Material* material;

    Qubes::EditMenu* menu;

    UnityEngine::Color color;
    int type;
    int hitAction;
    float size;
    bool locked;

    int index; // for editing in config
)

// a default cube, but cuttable and interactible
DECLARE_CLASS_CUSTOM(Qubes, Cube, Qubes::DefaultCube,
    DECLARE_INSTANCE_METHOD(void, Update);
    DECLARE_INSTANCE_METHOD(void, LateUpdate);

    public:

    custom_types::Helpers::Coroutine respawnCoroutine();

    void init(UnityEngine::Color color, int cubeType, int onHit, float cubeSize, bool locked, int index);
    void handleCut(GlobalNamespace::Saber* saber, UnityEngine::Vector3 cutPoint, UnityEngine::Quaternion orientation, UnityEngine::Vector3 cutDirVec);

    bool deletePressed();
    void editPressed();

    GlobalNamespace::BoxCuttableBySaber* hitbox;

    GlobalNamespace::VRController* controller;
    UnityEngine::RaycastHit hit;
    UnityEngine::Vector3 grabPos;
    UnityEngine::Quaternion grabRot;
)