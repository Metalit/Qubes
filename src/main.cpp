#include "main.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "questui/shared/QuestUI.hpp"

#include "config-utils/shared/config-utils.hpp"
#include "custom-types/shared/register.hpp"

#include "UnityEngine/SceneManagement/SceneManager.hpp"

#include "GlobalNamespace/OVRInput_Button.hpp"
#include "GlobalNamespace/HMMainThreadDispatcher.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/EffectPoolsManualInstaller.hpp"

#include "UnityEngine/Physics.hpp"
#include "UnityEngine/Collider.hpp"
#include "UnityEngine/Random.hpp"
#include "UnityEngine/MaterialPropertyBlock.hpp"
#include "GlobalNamespace/PauseAnimationController.hpp"
#include "GlobalNamespace/ColorManager.hpp"
#include "GlobalNamespace/ColorType.hpp"
#include "GlobalNamespace/NoteDebrisPhysics.hpp"
#include "GlobalNamespace/MaterialPropertyBlockController.hpp"

using namespace GlobalNamespace;

ModInfo modInfo;
DEFINE_CONFIG(ModConfig);

std::vector<Cube*> cubeArr;
NoteDebris* debrisPrefab;
UnityEngine::Color lastColor;
VRUIControls::VRPointer* pointer;
HapticFeedbackController* haptics;
PauseController* pauser;
bool inMenu, inGameplay, created = false;

std::vector<QubesConfig> QubesConfigs = {
    QubesConfig("qubes"),
    QubesConfig("qubes_default",
        {CubeInfo({-3.7, 1, 1.2}, {0, -0.5, 0, 0.866}, {0.5, 0.5, 0.5, 1}, 2, 0, 1, false)})
};

// QubesConfigs.push_back(Cubes);
// QubesConfigs.push_back(DefCubes);

UnityEngine::GameObject* gameNote;
DefaultCube* defaultCube;
const std::vector<OVRInput::Button> buttons = {
    OVRInput::Button::PrimaryHandTrigger,
    OVRInput::Button::One,
    OVRInput::Button::Two
};

Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

// currently unused but can be useful
void logChildren(UnityEngine::Transform* t, std::string indent) {
    int num = t->get_childCount();
    getLogger().info("%s%s has %i child%s", indent.c_str(), to_utf8(csstrtostr(t->get_gameObject()->get_name())).c_str(), num, num == 1? "" : "ren");
    auto arr = t->get_gameObject()->GetComponents<UnityEngine::MonoBehaviour*>();
    for(int i = 0; i < arr->Length(); i++) {
        getLogger().info("%s  Cmpnt: %s", indent.c_str(), to_utf8(csstrtostr(arr->get(i)->GetScriptClassName())).c_str());
    }
    for(int i = 0; i < num; i++) {
        logChildren(t->GetChild(i), indent + "  ");
    }
}
void logHierarchy() {
    auto objects = UnityEngine::SceneManagement::SceneManager::GetActiveScene().GetRootGameObjects();
    for(int i = 0; i < objects->Length(); i++) {
        getLogger().info("Root object: %s", to_utf8(csstrtostr(objects->get(i)->get_name())).c_str());
        logChildren(objects->get(i)->get_transform(), "  ");
    }
}

// make cubes
// idk if there's a better way to do these macros
#define threeVal(arr) {arr[0], arr[1], arr[2]}
#define fourVal(arr) {arr[0], arr[1], arr[2], arr[3]}
Cube* makeCube(CubeInfo info, QubesConfig& cfg, int index) {
    auto pos = UnityEngine::Vector3 threeVal(info.pos);
    auto rot = UnityEngine::Quaternion fourVal(info.rot);
    auto color = UnityEngine::Color threeVal(info.color);
    auto ob = UnityEngine::Object::Instantiate(gameNote, pos, rot);
    UnityEngine::Object::DontDestroyOnLoad(ob);
    auto cube = ob->AddComponent<Cube*>();
    // needs to be set active before init
    ob->set_active(true);
    cube->init(color, info.type, info.hitAction, info.size, info.locked, cfg, index);
    return cube;
}
DefaultCube* makeDefaultCube(CubeInfo info, QubesConfig& cfg, int index) {
    auto pos = UnityEngine::Vector3 threeVal(info.pos);
    auto rot = UnityEngine::Quaternion fourVal(info.rot);
    auto color = UnityEngine::Color threeVal(info.color);
    auto ob = UnityEngine::Object::Instantiate(gameNote, pos, rot);
    UnityEngine::Object::DontDestroyOnLoad(ob);
    auto cube = ob->AddComponent<DefaultCube*>();
    // needs to be set active before init
    ob->set_active(true);
    cube->init(color, info.type, info.hitAction, info.size, info.locked, cfg, index);
    return cube;
}
#undef threeVal
#undef fourVal

// Hooks
MAKE_HOOK_MATCH(SceneChanged, &UnityEngine::SceneManagement::SceneManager::Internal_ActiveSceneChanged, void, UnityEngine::SceneManagement::Scene prevScene, UnityEngine::SceneManagement::Scene nextScene) {
    SceneChanged(prevScene, nextScene);
    // names = MainMenu, GameCore (QuestInit, EmptyTransition, HealthWarning, ShaderWarmup)
    if(nextScene && nextScene.IsValid() && to_utf8(csstrtostr(nextScene.get_name())) == "ShaderWarmup") {
        // fix a few soft restart bugs
        pointer = nullptr;
        if(!created) {
            getLogger().info("Creating cubes");
            // scene where the cube transform model is available
            auto transform = UnityEngine::GameObject::Find(il2cpp_utils::createcsstr("NormalGameNote"))->get_transform()->Find(il2cpp_utils::createcsstr("NoteCube"));
            // need to instantiate it into our own object so it stays available for creating cubes on demand
            gameNote = UnityEngine::Object::Instantiate(transform)->get_gameObject();
            UnityEngine::Object::DontDestroyOnLoad(gameNote);
            gameNote->set_active(false);

            // cubes config loaded in the config init
            for(auto info : QubesConfigs[0].cubes)
                cubeArr.push_back(makeCube(info, QubesConfigs[0], cubeArr.size()));

            // only use the first cube in default cubes
            defaultCube = makeDefaultCube(QubesConfigs[1].cubes[0], QubesConfigs[1], 0);

            created = true;
        }
    }
    if(nextScene && to_utf8(csstrtostr(nextScene.get_name())) == "MainMenu") {
        inMenu = true;
        // get pointer every time, since it changes in pause
        pointer = UnityEngine::Resources::FindObjectsOfTypeAll<VRUIControls::VRPointer*>()->get(0);
        
        // activate or deactivate all cubes based on ShowInMenu
        auto active = getModConfig().ShowInMenu.GetValue();
        for(auto cube : cubeArr)
            cube->setActive(active);
        // disable default cube because we are not inside the settings
        defaultCube->setActive(false);
    } else inMenu = false;

    if(nextScene && to_utf8(csstrtostr(nextScene.get_name())) == "GameCore") {
        inGameplay = true;
        // activate or deactivate all cubes based on ShowInLevel
        auto active = getModConfig().ShowInLevel.GetValue();
        for(auto cube : cubeArr) {
            cube->setActive(active);
            cube->setMenuActive(false);
        }

        pauser = UnityEngine::Resources::FindObjectsOfTypeAll<PauseController*>()->get(0);
        // only need to get the addresses once
        if(!haptics)
            haptics = UnityEngine::Resources::FindObjectsOfTypeAll<HapticFeedbackController*>()->get(0);
        if(!debrisPrefab)
            debrisPrefab = UnityEngine::Resources::FindObjectsOfTypeAll<EffectPoolsManualInstaller*>()->get(0)->noteDebrisHDPrefab;
    } else inGameplay = false;
}

#define toVector3(vector4) UnityEngine::Vector3(vector4.x, vector4.y, vector4.z)
#define toVector4(vector3) UnityEngine::Vector4(vector3.x, vector3.y, vector3.z, 0)
MAKE_HOOK_MATCH(DebrisInit, &NoteDebris::Init, void, NoteDebris* self, ColorType colorType, UnityEngine::Vector3 notePos, UnityEngine::Quaternion noteRot, UnityEngine::Vector3 noteMoveVec, UnityEngine::Vector3 noteScale, UnityEngine::Vector3 positionOffset, UnityEngine::Quaternion rotationOffset, UnityEngine::Vector3 cutPoint, UnityEngine::Vector3 cutNormal, UnityEngine::Vector3 force, UnityEngine::Vector3 torque, float lifeTime) {
    // leave it normal if the debris is not from a cube
    if(colorType != ColorType::_get_None()) {
        DebrisInit(self, colorType, notePos, noteRot, noteMoveVec, noteScale, positionOffset, rotationOffset, cutPoint, cutNormal, force, torque, lifeTime); // I have no idea why this doesn't work
        return;
    }
    // don't call what we're hooking to avoid custom debris
    UnityEngine::Quaternion quaternion = UnityEngine::Quaternion::Inverse(noteRot);
    UnityEngine::Vector3 vector = quaternion * (cutPoint - notePos);
    UnityEngine::Vector3 vector2 = quaternion * cutNormal;
    float sqrMagnitude = vector.get_sqrMagnitude();
    if (sqrMagnitude > self->maxCutPointCenterDistance * self->maxCutPointCenterDistance)
        vector = self->maxCutPointCenterDistance * vector / sqrt(sqrMagnitude);
    UnityEngine::Vector4 vector3 = {vector2.x, vector2.y, vector2.z, 0};
    vector3.w = 0 - UnityEngine::Vector3::Dot(vector2, vector);
    float num = sqrt(UnityEngine::Vector4::Dot(vector3, vector3));
    UnityEngine::Vector3 zero = UnityEngine::Vector3::get_zero();
    int num2 = self->_get__meshVertices()->Length();
    for (int i = 0; i < num2; i++) {
        UnityEngine::Vector3 vector4 = self->_get__meshVertices()->get(i);
        float num3 = UnityEngine::Vector3::Dot(toVector3(vector3), vector4) + vector3.w;
        if (num3 < 0) {
            float num4 = num3 / num;
            UnityEngine::Vector3 vector5 = vector4 - toVector3(vector3) * num4;
            zero = zero + vector5 / num2;
        } else
            zero = zero + vector4 / num2;
    }
    UnityEngine::Quaternion quaternion2 = rotationOffset * noteRot;
    UnityEngine::Transform* obj = self->get_transform();
    obj->SetPositionAndRotation(rotationOffset * notePos + positionOffset + quaternion2 * zero, quaternion2);
    obj->set_localScale(noteScale);
    self->meshTransform->set_localPosition(-zero);
    self->physics->Init(force, torque);
    UnityEngine::Color value = self->colorManager->ColorForType(colorType);
    UnityEngine::MaterialPropertyBlock* materialPropertyBlock = self->materialPropertyBlockController->get_materialPropertyBlock();
    materialPropertyBlock->Clear();
    materialPropertyBlock->SetColor(self->_get__colorID(), value);
    materialPropertyBlock->SetVector(self->_get__cutPlaneID(), vector3);
    materialPropertyBlock->SetVector(self->_get__cutoutTexOffsetID(), toVector4(UnityEngine::Random::get_insideUnitSphere()));
    materialPropertyBlock->SetFloat(self->_get__cutoutPropertyID(), 0);
    self->materialPropertyBlockController->ApplyChanges();
    self->lifeTime = lifeTime;
    self->elapsedTime = 0;
    // set custom color if the debris is not from a normal game note
    auto mat = self->materialPropertyBlockController->materialPropertyBlock;
    if(mat)
        mat->SetColor(self->_get__colorID(), lastColor);
}

// just an object that is guaranteed active
MAKE_HOOK_MATCH(AnUpdate, &HMMainThreadDispatcher::Update, void, HMMainThreadDispatcher* self) {
    AnUpdate(self);
    // don't listen for buttons in gameplay
    if(!pointer || (!inMenu))
        return;
    int i = 0; // keep track of index
    for(auto button : buttons) {
        bool lbut = OVRInput::GetDown(button, OVRInput::Controller::LTouch);
        bool rbut = OVRInput::GetDown(button, OVRInput::Controller::RTouch);
        bool isRight = pointer->_get__lastControllerUsedWasRight();
        if((lbut && !isRight) || (rbut && isRight)) {
            // check button with configured buttons and controller with configured controllers for all three
            if(i == getModConfig().BtnDel.GetValue() && (getModConfig().CtrlDel.GetValue() == 2 || getModConfig().CtrlDel.GetValue() == (isRight? 1 : 0))) {
                getLogger().info("delete pressed");
                // only delete one cube
                bool deleted = false;
                // physics raycast allows interaction through ui elements
                UnityEngine::RaycastHit hit;
                if(UnityEngine::Physics::Raycast(pointer->get_vrController()->get_position(), pointer->get_vrController()->get_forward(), hit, 100)) {
                    // iterate through cubes to find the one to be deleted
                    auto hitTransform = hit.get_collider()->get_transform();
                    for(auto iter = cubeArr.begin(); iter != cubeArr.end(); ++iter) {
                        if(!deleted && (*iter)->deletePressed(hitTransform)) {
                            // destroyed and removed from config in deletePressed
                            cubeArr.erase(iter);
                            deleted = true;
                            // subtract one from the iterator since we removed one from the array
                            --iter;
                        } else if(deleted) {
                            // decrement indices of all later cubes
                            (*iter)->index--;
                        }
                    }
                }
            }
            if(i == getModConfig().BtnMake.GetValue() && (getModConfig().CtrlMake.GetValue() == 2 || getModConfig().CtrlMake.GetValue() == (isRight? 1 : 0))) {
                getLogger().info("create pressed");
                // use pointer to get creation position
                auto ctrlr = pointer->get_vrController();
                auto pos = ctrlr->get_position() + (ctrlr->get_forward().get_normalized() * (1.5 * getModConfig().CreateDist.GetValue()));
                auto rot = getModConfig().CreateRot.GetValue() ? ctrlr->get_rotation() : UnityEngine::Quaternion::get_identity();
                // create
                auto info = CubeInfo(pos, rot, defaultCube->getColor(), defaultCube->getType(), defaultCube->getHitAction(), defaultCube->getSize(), defaultCube->getLocked());
                cubeArr.push_back(makeCube(info, QubesConfigs[0], cubeArr.size()));
                QubesConfigs[0].AddCube(info);
            }
            if(i == getModConfig().BtnEdit.GetValue() && (getModConfig().CtrlEdit.GetValue() == 2 || getModConfig().CtrlEdit.GetValue() == (isRight? 1 : 0))) {
                getLogger().info("edit pressed");
                // physics raycast allows interaction through ui elements
                UnityEngine::RaycastHit hit;
                if(UnityEngine::Physics::Raycast(pointer->get_vrController()->get_position(), pointer->get_vrController()->get_forward(), hit, 100)) {
                    auto hitTransform = hit.get_collider()->get_transform();
                    // cubes handle it internally
                    for(auto cube : cubeArr) {
                        cube->editPressed(hitTransform);
                    }
                }
            }
        }
        i++;
    }
}

MAKE_HOOK_MATCH(Pause, &PauseController::Pause, void, PauseController* self) {
    Pause(self);
    // get pointer every time, since it changes in menu (and also every pause or something)
    auto arr = UnityEngine::Resources::FindObjectsOfTypeAll<VRUIControls::VRPointer*>();
    if(arr->Length() < 2)
        return;
    pointer = arr->get(1);
    inMenu = true;
}

MAKE_HOOK_MATCH(Resume, &PauseAnimationController::StartResumeFromPauseAnimation, void, PauseAnimationController* self) {
    Resume(self);
    // deactivate all menus and let all cubes only be cut after the saber registers having moved
    for(auto cube : cubeArr) {
        cube->setMenuActive(false);
        cube->setCuttableDelay(false, 0);
        cube->setCuttableDelay(true, 0.75);
    }
    inMenu = false;
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getLogger().info("Completed setup!");
}

extern "C" void load() {
    il2cpp_functions::Init();

    custom_types::Register::AutoRegister();

    getModConfig().Init(modInfo);
    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsFlowCoordinator<ModSettings*>(modInfo);
    QuestUI::Register::RegisterMainMenuModSettingsFlowCoordinator<ModSettings*>(modInfo);

    getLogger().info("Installing hooks...");
    LoggerContextObject logger = getLogger().WithContext("load");
    INSTALL_HOOK(logger, SceneChanged);
    INSTALL_HOOK(logger, DebrisInit);
    INSTALL_HOOK(logger, AnUpdate);
    INSTALL_HOOK(logger, Pause);
    INSTALL_HOOK(logger, Resume);
    getLogger().info("Installed all hooks!");
}