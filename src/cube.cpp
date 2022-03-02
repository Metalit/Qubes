#include "main.hpp"
#include "assets.hpp"

#include "GlobalNamespace/ILevelRestartController.hpp"
#include "GlobalNamespace/IReturnToMenuController.hpp"
#include "GlobalNamespace/CuttableBySaber_WasCutBySaberDelegate.hpp"
#include "GlobalNamespace/ColorType.hpp"
#include "GlobalNamespace/INoteDebrisDidFinishEvent.hpp"
#include "GlobalNamespace/ILazyCopyHashSet_1.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "GlobalNamespace/SaberTypeObject.hpp"
#include "GlobalNamespace/SaberTypeExtensions.hpp"
#include "Libraries/HM/HMLib/VR/HapticPresetSO.hpp"

#include "UnityEngine/EventSystems/PointerEventData.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/Random.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Mathf.hpp"

#include "System/Collections/IEnumerator.hpp"
#include "UnityEngine/WaitForSeconds.hpp"

#include "questui/shared/BeatSaberUI.hpp"

DEFINE_TYPE(Qubes, DefaultCube);
DEFINE_TYPE(Qubes, Cube);
DEFINE_TYPE(Qubes, EditMenu);

using namespace Qubes;
using namespace QuestUI;

#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/AdditionalCanvasShaderChannels.hpp"
#include "UnityEngine/RenderMode.hpp"

#pragma region nonClass
const std::vector<std::string> cubeTypes = { "Blank", "Dot", "Arrow" };
const std::vector<std::string> cutEvents = { "None", "Pause", "Restart", "Menu", "Crash" };

#define START_CO(coroutine) StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))
#define COROUTINE(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->START_CO(coroutine)

#define GET_SPRITE(name) auto name##_arr = Array<uint8_t>::NewLength(name##_png::getLength()); \
uint8_t* name##_arr_vals = (uint8_t*) name##_arr->values; name##_arr_vals = name##_png::getData(); \
auto name##_sprite = BeatSaberUI::ArrayToSprite(name##_arr);

custom_types::Helpers::Coroutine crashCoroutine() {
    co_yield (System::Collections::IEnumerator*) UnityEngine::WaitForSeconds::New_ctor(0.5);
    getLogger().info("Crashing");
    SAFE_ABORT();
    co_return;
}
custom_types::Helpers::Coroutine deleteCoroutine(GlobalNamespace::NoteDebris* debris) {
    co_yield (System::Collections::IEnumerator*) UnityEngine::WaitForSeconds::New_ctor(1.5);
    getLogger().info("Deleting debris");
    auto ob = debris->get_gameObject();
    if(ob)
        UnityEngine::Object::Destroy(ob);
    co_return;
}

void spawnDebris(UnityEngine::Vector3 cutPoint, UnityEngine::Vector3 cutNormal, float saberSpeed, UnityEngine::Vector3 saberDir, UnityEngine::Vector3 notePos, UnityEngine::Quaternion noteRotation, UnityEngine::Vector3 noteScale, UnityEngine::Color color) {
    GlobalNamespace::NoteDebris* noteDebris = UnityEngine::Object::Instantiate<GlobalNamespace::NoteDebris*>(debrisPrefab);
    GlobalNamespace::NoteDebris* noteDebris2 = UnityEngine::Object::Instantiate<GlobalNamespace::NoteDebris*>(debrisPrefab);

    UnityEngine::Vector3 vector = saberDir * (saberSpeed * 0.1);
    if(cutPoint.y < 1.3)
        vector.y = std::min(vector.y, (float)0);
    else if(cutPoint.y > 1.3)
        vector.y = std::max(vector.y, (float)0);

    UnityEngine::Quaternion rotation = UnityEngine::Quaternion::get_identity();
    UnityEngine::Vector3 force = rotation * (-(cutNormal + UnityEngine::Random::get_onUnitSphere() * 0.1) * 2 + vector);
    UnityEngine::Vector3 force2 = rotation * ((cutNormal + UnityEngine::Random::get_onUnitSphere() * 0.1) * 2 + vector);
    UnityEngine::Vector3 vector2 = rotation * UnityEngine::Vector3::Cross(cutNormal, saberDir) * 0.5;
    
    lastColor = color;
    noteDebris->Init(GlobalNamespace::ColorType::_get_None(), notePos, noteRotation, UnityEngine::Vector3::get_zero(), noteScale, UnityEngine::Vector3::get_zero(), rotation, cutPoint, -cutNormal, force, -vector2, 2);
    noteDebris2->Init(GlobalNamespace::ColorType::_get_None(), notePos, noteRotation, UnityEngine::Vector3::get_zero(), noteScale, UnityEngine::Vector3::get_zero(), rotation, cutPoint, cutNormal, force2, vector2, 2);
    noteDebris->START_CO(deleteCoroutine(noteDebris));
    noteDebris2->START_CO(deleteCoroutine(noteDebris2));

    getLogger().info("Custom debris spawned"); // actually just copied from the game, with some unnecessary variables removed
}
#pragma endregion

#pragma region defaultCube
UnityEngine::Transform* DefaultCube::findTransform(std::string_view name) {
    // finds children transforms by name
    return get_transform()->Find(name);
}

void DefaultCube::init(UnityEngine::Color color, int cubeType, int onHit, float cubeSize, bool lock, QubesConfig& cfg, int cfg_index) {
    getLogger().info("Initializing cube");

    material = GetComponent<UnityEngine::MeshRenderer*>()->get_material();

    UnityEngine::Object::Destroy(findTransform("BigCuttable")->get_gameObject());

    menuActive = false;
    typeSet = false;
    // shows dot for one frame because it doesn't render if you don't
    findTransform("NoteCircleGlow")->get_gameObject()->set_active(true);
    type = cubeType;

    config = &cfg;
    index = cfg_index;
    setHitAction(onHit);
    setLocked(lock);
    setColor(color);
    setSize(cubeSize);

    auto renderers = GetComponentsInChildren<UnityEngine::Renderer*>();
    for(int i = 0; i < renderers.Length(); i++) {
        renderers[i]->set_enabled(true);
    }
    findTransform("SmallCuttable")->GetComponent<GlobalNamespace::BoxCuttableBySaber*>()->set_colliderSize({0.5, 0.5, 0.5});

    get_gameObject()->set_active(true);
}

void DefaultCube::makeMenu() {
    getLogger().info("Creating menu");
    auto go = BeatSaberUI::CreateCanvas();
    UnityEngine::Object::DontDestroyOnLoad(go);
    // makes it render on the same layer as the pause menu
    go->GetComponent<UnityEngine::Canvas*>()->set_sortingOrder(31);
    menu = go->AddComponent<EditMenu*>();
    menu->init(this); // mostly handled here
}

void DefaultCube::setActive(bool active) {
    get_gameObject()->set_active(active);
    if(!typeSet && active)
        setType(type);
}

void DefaultCube::setMenuActive(bool active) {
    if(!menu) {
        makeMenu();
        menu->closeButton->get_gameObject()->set_active(false);
        menu->lockButton->get_gameObject()->set_active(false);
        menu->get_transform()->set_localRotation(UnityEngine::Quaternion::get_identity());
    }
    menu->get_gameObject()->set_active(active);
    menuActive = active;
}

void DefaultCube::save() {
    config->SetCubeValue(index, CubeInfo(this));
}

void DefaultCube::setColor(UnityEngine::Color color) {
    this->color = color;
    material->set_color(color);
    if(menu)
        menu->colButtonController->SetColor(color);
}

void DefaultCube::setType(int cubeType) {
    type = cubeType;
    typeSet = true;
    findTransform("NoteArrow")->get_gameObject()->set_active(false);
    // 0: nothing, 1: dot, 2: arrow
    findTransform("NoteCircleGlow")->get_gameObject()->set_active(cubeType == 1);
    findTransform("NoteArrowGlow")->get_gameObject()->set_active(cubeType == 2);
}

void DefaultCube::setSize(float newSize) {
    size = newSize;
    get_transform()->set_localScale({size, size, size});
    if(menu) {
        float inv_size = 0.03/size;
        menu->get_transform()->set_localScale({inv_size, inv_size, inv_size});
    }
}
#pragma endregion

#pragma region cube
custom_types::Helpers::Coroutine Cube::respawnCoroutine() {
    co_yield (System::Collections::IEnumerator*) UnityEngine::WaitForSeconds::New_ctor(getModConfig().RespawnTime.GetValue());
    auto ob = get_gameObject();
    // don't respawn if in menu and show in menu is disabled
    if(ob && !(inMenu && !getModConfig().ShowInMenu.GetValue()))
        ob->set_active(true);
    co_return;
}

custom_types::Helpers::Coroutine Cube::cuttableCoroutine(bool cuttable, float seconds) {
    co_yield (System::Collections::IEnumerator*) UnityEngine::WaitForSeconds::New_ctor(seconds);
    if(hitbox)
        hitbox->set_canBeCut(cuttable);
    co_return;
}

void Cube::setCuttableDelay(bool cuttable, float seconds) {
    if(seconds == 0)
        hitbox->set_canBeCut(cuttable);
    else
        COROUTINE(cuttableCoroutine(cuttable, seconds));
}

void Cube::init(UnityEngine::Color color, int cubeType, int onHit, float cubeSize, bool lock, QubesConfig& cfg, int cfg_index) {
    DefaultCube::init(color, cubeType, onHit, cubeSize, lock, cfg, cfg_index);

    hitbox = findTransform("SmallCuttable")->GetComponent<GlobalNamespace::BoxCuttableBySaber*>();

    hitbox->add_wasCutBySaberEvent(il2cpp_utils::MakeDelegate<GlobalNamespace::CuttableBySaber::WasCutBySaberDelegate*>(classof(GlobalNamespace::CuttableBySaber::WasCutBySaberDelegate*),
      (std::function<void(GlobalNamespace::Saber* saber, UnityEngine::Vector3 cutPoint, UnityEngine::Quaternion orientation, UnityEngine::Vector3 cutDirVec)>)
      [this](GlobalNamespace::Saber* saber, UnityEngine::Vector3 cutPoint, UnityEngine::Quaternion orientation, UnityEngine::Vector3 cutDirVec){
        this->handleCut(saber, cutPoint, orientation, cutDirVec);
    }));
}

void Cube::setMenuActive(bool active) {
    bool first = menu == nullptr;
    DefaultCube::setMenuActive(active);
    if(first) {
        menu->closeButton->get_gameObject()->set_active(true);
        menu->lockButton->get_gameObject()->set_active(true);
    }
}

void Cube::Update() {
    // avoid moving underground
    if(get_transform()->get_position().y < 0) {
        auto pos = get_transform()->get_position();
        get_transform()->set_position({pos.x, 0, pos.z});
    }
    if(!typeSet)
        setType(type);

    if(!pointer || locked)
        return;

    // check if being grabbed
    bool wasGrabbing = !(controller == nullptr);
    if(pointer->get_vrController()->get_triggerValue() > 0.9) {
        // pointerData can be null when not loaded (aka on soft restarts, generally)
        if(controller == pointer->get_vrController() || !pointer->pointerData)
            return;
        // check if pointer is on the cube
        if(pointer->pointerData->pointerCurrentRaycast.get_gameObject() == hitbox->get_gameObject()) {
            controller = pointer->get_vrController();
            grabPos = pointer->get_vrController()->get_transform()->InverseTransformPoint(get_transform()->get_position());
            grabRot = UnityEngine::Quaternion::Inverse(pointer->get_vrController()->get_transform()->get_rotation()) * get_transform()->get_rotation();
        } else
            controller = nullptr;
    } else
        controller = nullptr;
    // save config on release
    if(wasGrabbing && !controller)
        save();
}

void Cube::LateUpdate() {
    if(!controller || locked || !pointer)
        return;
    // thumbstick movement
    if(pointer->_get__lastControllerUsedWasRight() == !getModConfig().LeftThumbMove.GetValue()) {
        float diff = controller->get_verticalAxisValue() * UnityEngine::Time::get_unscaledDeltaTime() * getModConfig().MoveSpeed.GetValue();
        // no movement if too close
        if(grabPos.get_magnitude() < 0.5 * size && diff > 0)
            diff = 0;
        grabPos = grabPos - (UnityEngine::Vector3::get_forward() * diff);
    }

    // thumbstick rotation
    if(pointer->_get__lastControllerUsedWasRight() == getModConfig().LeftThumbMove.GetValue()) {
        // scale values to a reasonable "1" speed
        float v_move = -20 * controller->get_verticalAxisValue() * UnityEngine::Time::get_unscaledDeltaTime() * getModConfig().RotSpeed.GetValue();
        float h_move = -20 * controller->get_horizontalAxisValue() * UnityEngine::Time::get_unscaledDeltaTime() * getModConfig().RotSpeed.GetValue();
        auto extraRot = UnityEngine::Quaternion::Euler({0, h_move, 0}) * UnityEngine::Quaternion::Euler({v_move, 0, 0});
        grabRot = grabRot * extraRot;
    }

    auto pos = controller->get_transform()->TransformPoint(grabPos);
    auto rot = controller->get_transform()->get_rotation() * grabRot;
    
    get_transform()->set_position(UnityEngine::Vector3::Lerp(get_transform()->get_position(), pos, 10 * UnityEngine::Time::get_unscaledDeltaTime()));
    get_transform()->set_rotation(UnityEngine::Quaternion::Slerp(get_transform()->get_rotation(), rot, 5 * UnityEngine::Time::get_unscaledDeltaTime()));
}

#include "GlobalNamespace/GamePause.hpp"
#include "GlobalNamespace/PauseMenuManager.hpp"
#include "GlobalNamespace/BeatmapObjectManager.hpp"
#include "System/Action.hpp"

void Cube::handleCut(GlobalNamespace::Saber* saber, UnityEngine::Vector3 cutPoint, UnityEngine::Quaternion orientation, UnityEngine::Vector3 cutDirVec) {
    getLogger().info("Qube cut");
    
    if(getModConfig().ReqDirection.GetValue() && type == 2) {
        using UnityEngine::Mathf;
        auto cutDirVec2 = get_transform()->InverseTransformVector(cutDirVec);
        bool flag = Mathf::Abs(cutDirVec2.z) > Mathf::Abs(cutDirVec2.x) * 10 && Mathf::Abs(cutDirVec2.z) > Mathf::Abs(cutDirVec2.y) * 10;
        float cutDirAngle = Mathf::Atan2(cutDirVec2.y, cutDirVec2.x) * 57.29578;
        float cutAngleTolerance = 50; // 60 default, 40 on strict angles
        bool dirOk = !flag && cutDirAngle > -90 - cutAngleTolerance && cutDirAngle < -90 + cutAngleTolerance;
        if(!dirOk)
            return;
    }
    // avoid nullptr
    if(debrisPrefab && getModConfig().Debris.GetValue())
        spawnDebris(cutPoint, orientation * UnityEngine::Vector3::get_up(), saber->get_bladeSpeed(), cutDirVec.get_normalized(), get_transform()->get_position(), get_transform()->get_rotation(), get_transform()->get_localScale(), color);
    
    // give haptic feedback
    static Libraries::HM::HMLib::VR::HapticPresetSO* hapticPreset = Libraries::HM::HMLib::VR::HapticPresetSO::New_ctor();
    hapticPreset->strength = getModConfig().Vibration.GetValue();
    haptics->PlayHapticFeedback(GlobalNamespace::SaberTypeExtensions::Node(saber->saberType->get_saberType()), hapticPreset);

    if(getModConfig().RespawnTime.GetValue() > 0) {
        get_gameObject()->set_active(false);
        COROUTINE(respawnCoroutine());
    } else {
        // avoid cutting at an unreasonable rate
        hitbox->set_canBeCut(false);
        setCuttableDelay(true, 0.1);
    }
    // no hit actions in menu
    if(!inGameplay)
        return;
    switch (hitAction) {
        case 1:
            getLogger().info("pause");
            // pauser->Pause(); // doesn't work with fish utils's pause tweaks
            if(pauser->get_canPause()) {
                pauser->paused = true;
                pauser->gamePause->Pause();
                pauser->pauseMenuManager->ShowMenu();
                pauser->beatmapObjectManager->HideAllBeatmapObjects(true);
                pauser->beatmapObjectManager->PauseAllBeatmapObjects(true);
                if(pauser->didPauseEvent)
                    pauser->didPauseEvent->Invoke();
                pointer = UnityEngine::Resources::FindObjectsOfTypeAll<VRUIControls::VRPointer*>()[1];
                inMenu = true;
            }
            break;
        case 2:
            getLogger().info("restart");
            pauser->levelRestartController->RestartLevel();
            break;
        case 3:
            getLogger().info("menu");
            pauser->returnToMenuController->ReturnToMenu();
            break;
        case 4:
            getLogger().info("crash");
            COROUTINE(crashCoroutine());
            break;
        default:
            break;
    }
}

bool Cube::deletePressed(UnityEngine::Transform* hit) {
    if(locked)
        return false;
    if(hit == hitbox->get_transform()) {
        config->RemoveCube(index);
        if(menu)
            UnityEngine::Object::Destroy(menu->get_gameObject());
        UnityEngine::Object::Destroy(get_gameObject());
        return true;
    }
    return false;
}

void Cube::editPressed(UnityEngine::Transform* hit) {
    if(hit == hitbox->get_transform()) {
        if(!menu)
            makeMenu();
        if(menu->get_gameObject()->get_active()) {
            setMenuActive(false);
            return;
        }
        setMenuActive(true);

        // set rotation such that the normal points at the pointer pos
        float pointerYrot = pointer->get_vrController()->get_rotation().get_eulerAngles().y;
        auto pointerPos = pointer->get_vrController()->get_position();
        auto menuPos = menu->get_transform()->get_position();
        // 1.5 = menu x offset from center
        float ratio = 1.5 / UnityEngine::Vector2::Distance({pointerPos.x, pointerPos.z}, {menuPos.x, menuPos.z});
        float addAngle = ratio > -1 && ratio < 1 ? 90 - std::acos(ratio)*180/3.151492653589 : 45;
        if(addAngle > 45)
            addAngle = 45;
        menu->get_transform()->set_eulerAngles({0, pointerYrot + addAngle, 0});
    }
}
#pragma endregion

#pragma region editMenu
#include "HMUI/Touchable.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/UI/GraphicRaycaster.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"

void EditMenu::LateUpdate() {
    // lock non-vertical axis rotation relative to cube
    auto t = get_transform();
    float y = t->get_eulerAngles().y;
    t->set_eulerAngles({0, y, 0});
    // set the position to an x offset rotated based on y rotation
    auto pos = t->get_parent()->get_position();
    // 1.5 = x offset from center
    auto offsetVec = UnityEngine::Quaternion::AngleAxis(y, UnityEngine::Vector3::get_up()) * UnityEngine::Vector3{1.5, 0, 0};
    t->set_position(offsetVec + pos);
}

void setButtons(IncrementSetting* inc) {
    auto arr = inc->get_gameObject()->get_transform()->GetChild(1)->GetComponentsInChildren<UnityEngine::UI::Button*>();
    auto decButton = arr[0];
    auto incButton = arr[1];
    decButton->set_interactable(inc->CurrentValue != inc->MinValue);
    incButton->set_interactable(inc->CurrentValue != inc->MaxValue);
}
void EditMenu::init(DefaultCube* parent) {
    getLogger().info("menu init");
    auto background = get_gameObject()->AddComponent<Backgroundable*>();
    background->ApplyBackgroundWithAlpha("round-rect-panel", 0.5);
    background->background->set_raycastTarget(true);
    GetComponent<UnityEngine::Canvas*>()->set_sortingOrder(31);

    get_gameObject()->AddComponent<HMUI::Touchable*>();
    get_transform()->SetParent(parent->get_transform());
    get_transform()->set_localPosition({0, 0, 0});
    float inv_size = 0.03/parent->getSize();
    get_transform()->set_localScale({inv_size, inv_size, inv_size});
    // doesn't resize to contents on its own
    ((UnityEngine::RectTransform*) get_transform())->set_sizeDelta({75, 25});
    get_gameObject()->set_active(false);

    // value settings
    valVertical = BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
    valVertical->get_rectTransform()->set_anchoredPosition({-4, -2});
    valVertical->set_childControlWidth(true);
    valVertical->set_childControlHeight(true);
    valVertical->set_childForceExpandHeight(false);

    // lines at the end set the widths to 60
    typeInc = BeatSaberUI::CreateIncrementSetting(valVertical->get_transform(), "Cube Type", 0, 1, parent->getType(), 0, 2, [parent, this](int value){
        parent->setType(value);
        parent->save();
        this->typeInc->Text->SetText(cubeTypes[value]);
        setButtons(this->typeInc);
    });
    setButtons(typeInc);
    typeInc->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
    typeInc->Text->SetText(cubeTypes[parent->getType()]);

    eventInc = BeatSaberUI::CreateIncrementSetting(valVertical->get_transform(), "Hit Action", 0, 1, parent->getHitAction(), 0, 4, [parent, this](int value){
        parent->setHitAction(value);
        parent->save();
        this->eventInc->Text->SetText(cutEvents[value]);
        setButtons(this->eventInc);
    });
    setButtons(eventInc);
    eventInc->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
    eventInc->Text->SetText(cutEvents[parent->getHitAction()]);

    BeatSaberUI::CreateSliderSetting(valVertical->get_transform(), "Qube Size", 0.01, parent->getSize(), 0.25, 1.5, 0, [parent](float value){
        parent->setSize(value);
    })->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);

    // color settings
    colVertical = BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
    colVertical->get_rectTransform()->set_anchoredPosition({-4, -2});
    colVertical->set_childControlWidth(true);
    colVertical->set_childControlHeight(true);
    colVertical->set_childForceExpandHeight(false);
    
    auto colslider = BeatSaberUI::CreateSliderSetting(colVertical->get_transform(), "R", 1, parent->getColor().r * 255, 0, 255, 0, [parent](float value){
        parent->setColor({value/255, parent->getColor().g, parent->getColor().b, 1});
        parent->save();
    });
    colslider->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
    ((UnityEngine::RectTransform*) colslider->slider->get_transform())->set_sizeDelta({56, 0});

    colslider = BeatSaberUI::CreateSliderSetting(colVertical->get_transform(), "G", 1, parent->getColor().g * 255, 0, 255, 0, [parent](float value){
        parent->setColor({parent->getColor().r, value/255, parent->getColor().b, 1});
        parent->save();
    });
    colslider->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
    ((UnityEngine::RectTransform*) colslider->slider->get_transform())->set_sizeDelta({56, 0});

    colslider = BeatSaberUI::CreateSliderSetting(colVertical->get_transform(), "B", 1, parent->getColor().b * 255, 0, 255, 0, [parent](float value){
        parent->setColor({parent->getColor().r, parent->getColor().g, value/255, 1});
        parent->save();
    });
    colslider->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
    ((UnityEngine::RectTransform*) colslider->slider->get_transform())->set_sizeDelta({56, 0});
    
    colVertical->get_gameObject()->set_active(false);

    // viewed settings change and lock/close buttons
    closeButton = BeatSaberUI::CreateUIButton(get_transform(), "", "SettingsButton", [this](){
        this->get_gameObject()->set_active(false);
    });
    GET_SPRITE(close);
    GET_SPRITE(closeActive);
    BeatSaberUI::SetButtonSprites(closeButton, close_sprite, closeActive_sprite);
    ((UnityEngine::RectTransform*) closeButton->get_transform())->set_anchoredPosition({32, 7});
    ((UnityEngine::RectTransform*) closeButton->get_transform()->GetChild(0))->set_sizeDelta({5.5, 5.5});

    auto col_button = BeatSaberUI::CreateUIButton(get_transform(), "", "ColorPickerButtonSecondary", [this](){
        this->valVertical->get_gameObject()->set_active(!this->valVertical->get_gameObject()->get_active());
        this->colVertical->get_gameObject()->set_active(!this->colVertical->get_gameObject()->get_active());
    });
    ((UnityEngine::RectTransform*) col_button->get_transform())->set_anchoredPosition({32, 0});

    colButtonController = col_button->GetComponent<GlobalNamespace::ColorPickerButtonController*>();
    colButtonController->SetColor(parent->getColor());

    lockButton = BeatSaberUI::CreateUIButton(get_transform(), "", "SettingsButton", [this, parent](){
        parent->setLocked(!parent->getLocked());
        parent->save();
        if(parent->getLocked()) {
            GET_SPRITE(lock);
            GET_SPRITE(lockActive);
            BeatSaberUI::SetButtonSprites(lockButton, lock_sprite, lockActive_sprite);
        } else {
            GET_SPRITE(unlock);
            GET_SPRITE(unlockActive);
            BeatSaberUI::SetButtonSprites(lockButton, unlock_sprite, unlockActive_sprite);
        }
    });
    if(parent->getLocked()) {
        GET_SPRITE(lock);
        GET_SPRITE(lockActive);
        BeatSaberUI::SetButtonSprites(lockButton, lock_sprite, lockActive_sprite);
    } else {
        GET_SPRITE(unlock);
        GET_SPRITE(unlockActive);
        BeatSaberUI::SetButtonSprites(lockButton, unlock_sprite, unlockActive_sprite);
    }
    ((UnityEngine::RectTransform*) lockButton->get_transform())->set_anchoredPosition({32, -7});
    ((UnityEngine::RectTransform*) lockButton->get_transform()->GetChild(0))->set_sizeDelta({5.5, 5.5});
}
#pragma endregion