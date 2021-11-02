#pragma once
#include "extern/config-utils/shared/config-utils.hpp"

#include "extern/custom-types/shared/macros.hpp"
#include "extern/custom-types/shared/register.hpp"

#include "HMUI/ViewController.hpp"
#include "HMUI/FlowCoordinator.hpp"

#include "UnityEngine/Color.hpp"

namespace Qubes { class Cube; class DefaultCube; }

DECLARE_CLASS_CODEGEN(Qubes, GlobalSettings, HMUI::ViewController,
    DECLARE_OVERRIDE_METHOD(void, DidActivate, il2cpp_utils::FindMethodUnsafe("HMUI", "ViewController", "DidActivate", 3), bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
)

DECLARE_CLASS_CODEGEN(Qubes, CreationSettings, HMUI::ViewController,
    DECLARE_OVERRIDE_METHOD(void, DidActivate, il2cpp_utils::FindMethodUnsafe("HMUI", "ViewController", "DidActivate", 3), bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
    DECLARE_OVERRIDE_METHOD(void, DidDeactivate, il2cpp_utils::FindMethodUnsafe("HMUI", "ViewController", "DidDeactivate", 2), bool removedFromHierarchy, bool screenSystemDisabling);
)

DECLARE_CLASS_CODEGEN(Qubes, ButtonSettings, HMUI::ViewController,
    DECLARE_OVERRIDE_METHOD(void, DidActivate, il2cpp_utils::FindMethodUnsafe("HMUI", "ViewController", "DidActivate", 3), bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
)

DECLARE_CLASS_CODEGEN(Qubes, ModSettings, HMUI::FlowCoordinator,
    DECLARE_OVERRIDE_METHOD(void, DidActivate, il2cpp_utils::FindMethodUnsafe("HMUI", "FlowCoordinator", "DidActivate", 3), bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
    DECLARE_OVERRIDE_METHOD(void, BackButtonWasPressed, il2cpp_utils::FindMethodUnsafe("HMUI", "FlowCoordinator", "BackButtonWasPressed", 1), HMUI::ViewController* topViewController);
    
    Qubes::GlobalSettings* globalSettings;
    Qubes::CreationSettings* creationSettings;
    Qubes::ButtonSettings* buttonSettings;
)

struct CubeInfo {
    float pos [3];
    float rot [4];
    float color [4];
    int type;
    int hitAction;
    float size;
    bool locked;

    CubeInfo(UnityEngine::Vector3 pos, UnityEngine::Quaternion rot, UnityEngine::Color color, int type, int hitAction, float size, bool locked);
    CubeInfo(rapidjson::Value& obj);
    CubeInfo(Qubes::DefaultCube* cube);

    rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& allocator);
};

struct QubesConfig {
    Configuration* config;

    std::vector<CubeInfo> cubes;

    void Init(Configuration* cfg);

    void LoadValue();
    void SetValue();
    void SetCubeValue(int index, CubeInfo value);
    void AddCube(CubeInfo cube);
    void RemoveCube(int index);
};

extern QubesConfig Cubes;

DECLARE_CONFIG(ModConfig,

    CONFIG_VALUE(ShowInMenu, bool, "Show In Menu", true, "View and interact with the qubes in the main menu");
    CONFIG_VALUE(ShowInLevel, bool, "Show In Level", true, "View and interact with the qubes inside levels or the pause menu");
    CONFIG_VALUE(ReqDirection, bool, "Require Cut Direction", true, "Whether to require cuts to align with the direction of the qube arrow");
    CONFIG_VALUE(Debris, bool, "Spawn Debris", true, "Whether to spawn debris on cutting a qube");
    CONFIG_VALUE(RespawnTime, float, "Respawning Delay", 3, "The length of time to wait before respawning qubes");
    CONFIG_VALUE(CreateDist, float, "Creation Distance", 1, "The distance from the controller to create a cube");
    CONFIG_VALUE(CreateRot, bool, "Create Rotated", true, "Whether for qubes to be rotated on creation");
    CONFIG_VALUE(Vibration, float, "Vibration Strength", 1, "The strength of haptic feedback when cutting a qube");
    // controller settings
    // 0: side trigger, 1: lower button, 2: top button, 3: none
    CONFIG_VALUE(BtnMake, int, "Create Button", 0);
    CONFIG_VALUE(BtnEdit, int, "Edit Button", 2);
    CONFIG_VALUE(BtnDel, int, "Delete Button", 1);
    // left, right or both
    CONFIG_VALUE(CtrlMake, int, "  Controllers", 2);
    CONFIG_VALUE(CtrlEdit, int, "  Controllers", 2);
    CONFIG_VALUE(CtrlDel, int, "  Controllers", 2);

    CONFIG_VALUE(MoveSpeed, float, "Movement Speed", 1, "The speed that thumbstick controls move the qube");
    CONFIG_VALUE(RotSpeed, float, "Rotataion Speed", 1, "The speed that thumbstick controls rotate the qube");
    CONFIG_VALUE(LeftThumbMove, bool, "Swap Thumbsticks", false, "Default: right thumbstick moves, left thumbstick rotates");
    
    CONFIG_INIT_FUNCTION(
        CONFIG_INIT_VALUE(ShowInMenu);
        CONFIG_INIT_VALUE(ShowInLevel);
        CONFIG_INIT_VALUE(ReqDirection);
        CONFIG_INIT_VALUE(Debris);
        CONFIG_INIT_VALUE(RespawnTime);
        CONFIG_INIT_VALUE(CreateDist);
        CONFIG_INIT_VALUE(CreateRot);
        CONFIG_INIT_VALUE(Vibration);
        CONFIG_INIT_VALUE(BtnMake);
        CONFIG_INIT_VALUE(BtnEdit);
        CONFIG_INIT_VALUE(BtnDel);
        CONFIG_INIT_VALUE(CtrlMake);
        CONFIG_INIT_VALUE(CtrlEdit);
        CONFIG_INIT_VALUE(CtrlDel);
        CONFIG_INIT_VALUE(MoveSpeed);
        CONFIG_INIT_VALUE(RotSpeed);
        CONFIG_INIT_VALUE(LeftThumbMove);
        Cubes.Init(config);
    )
)