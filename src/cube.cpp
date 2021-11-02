#include "main.hpp"

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
#include "UnityEngine/Collider.hpp"
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

#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/AdditionalCanvasShaderChannels.hpp"
#include "UnityEngine/RenderMode.hpp"

#pragma region nonClass
#define START_CO(coroutine) StartCoroutine(reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(coroutine)))
#define COROUTINE(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->START_CO(coroutine)

custom_types::Helpers::Coroutine Cube::respawnCoroutine() {
    for (float i = 0; i < getModConfig().RespawnTime.GetValue(); i += 0.5) {
        co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.5));
    }
    auto ob = get_gameObject();
    // don't respawn if in menu and show in menu is disabled
    if(ob && !(inMenu && !getModConfig().ShowInMenu.GetValue()))
        ob->set_active(true);
    co_return;
}
custom_types::Helpers::Coroutine crashCoroutine() {
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.5));
    getLogger().info("Crashing. You did this.");
    CRASH_UNLESS(false);
    co_return;
}
custom_types::Helpers::Coroutine deleteCoroutine(GlobalNamespace::NoteDebris* debris) {
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(1.5));
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
    return get_transform()->Find(il2cpp_utils::createcsstr(name));
}

void DefaultCube::init(UnityEngine::Color color, int cubeType, int onHit, float cubeSize, bool lock, int cfg_index) {
    getLogger().info("Initializing cube");

    material = GetComponent<UnityEngine::MeshRenderer*>()->get_material();

    UnityEngine::Object::Destroy(findTransform("BigCuttable")->get_gameObject());

    // shows dot for one frame because it doesn't render if you don't
    findTransform("NoteCircleGlow")->get_gameObject()->set_active(true);
    typeSet = false;
    type = cubeType;

    hitAction = onHit;
    locked = false;
    index = cfg_index;
    setColor(color);
    setSize(cubeSize);

    auto renderers = GetComponentsInChildren<UnityEngine::Renderer*>();
    for(int i = 0; i < renderers->Length(); i++) {
        renderers->get(i)->set_enabled(true);
    }
    
    findTransform("SmallCuttable")->GetComponent<GlobalNamespace::BoxCuttableBySaber*>()->set_colliderSize({0.5, 0.5, 0.5});
}

void DefaultCube::makeMenu() {
    getLogger().info("Creating menu");
    // auto go = UnityEngine::GameObject::New_ctor();
    auto go = QuestUI::BeatSaberUI::CreateCanvas();
    getLogger().info("made canvas");
    UnityEngine::Object::DontDestroyOnLoad(go);
    menu = go->AddComponent<EditMenu*>();
    menu->init(this); // mostly handled here
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
void Cube::init(UnityEngine::Color color, int cubeType, int onHit, float cubeSize, bool lock, int cfg_index) {
    DefaultCube::init(color, cubeType, onHit, cubeSize, lock, cfg_index);

    hitbox = findTransform("SmallCuttable")->GetComponent<GlobalNamespace::BoxCuttableBySaber*>();

    hitbox->add_wasCutBySaberEvent(il2cpp_utils::MakeDelegate<GlobalNamespace::CuttableBySaber::WasCutBySaberDelegate*>(classof(GlobalNamespace::CuttableBySaber::WasCutBySaberDelegate*),
      (std::function<void(GlobalNamespace::Saber* saber, UnityEngine::Vector3 cutPoint, UnityEngine::Quaternion orientation, UnityEngine::Vector3 cutDirVec)>)
      [this](GlobalNamespace::Saber* saber, UnityEngine::Vector3 cutPoint, UnityEngine::Quaternion orientation, UnityEngine::Vector3 cutDirVec){
        this->handleCut(saber, cutPoint, orientation, cutDirVec);
    }));
}

void Cube::Update() {
    // avoid moving underground
    if(get_transform()->get_position().y < 0) {
        auto pos = get_transform()->get_position();
        get_transform()->set_position({pos.x, 0, pos.z});
    }
    if(!typeSet)
        setType(type);

    // check if being grabbed
    if(!pointer || locked)
        return;
    bool wasGrabbing = !(controller == nullptr);
    if(pointer->get_vrController()->get_triggerValue() > 0.9) {
        // getLogger().info("Pointer on %s", to_utf8(csstrtostr(pointer->pointerData->pointerCurrentRaycast.get_gameObject()->get_name())).c_str());
        if(controller == pointer->get_vrController())
            return;
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
        Cubes.SetCubeValue(index, CubeInfo(this));
}

void Cube::LateUpdate() {
    if(!controller || locked)
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

    get_gameObject()->set_active(false);
    COROUTINE(respawnCoroutine());
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
        case 4: // set saber color
            getLogger().info("saber");
            break;
        case 5:
            getLogger().info("crash");
            COROUTINE(crashCoroutine());
            break;
        default:
            break;
    }
}

bool Cube::deletePressed() {
    if(locked)
        return false;
    // physics raycast allows interaction through ui elements
    if(UnityEngine::Physics::Raycast(pointer->get_vrController()->get_position(), pointer->get_vrController()->get_forward(), hit, 100)) {
        if(hit.get_collider()->get_transform() == hitbox->get_transform()) {
            if(menu)
                UnityEngine::Object::Destroy(menu->get_gameObject());
            UnityEngine::Object::Destroy(get_gameObject());
            return true;
        }
    }
    return false;
}

void Cube::editPressed() {
    if(!menu)
        makeMenu();
    // physics raycast allows interaction through ui elements
    if(UnityEngine::Physics::Raycast(pointer->get_vrController()->get_position(), pointer->get_vrController()->get_forward(), hit, 100)) {
        if(hit.get_collider()->get_transform() == hitbox->get_transform()) {
            if(menu->get_gameObject()->get_active()) {
                menu->get_gameObject()->set_active(false);
                return;
            }
            menu->get_gameObject()->set_active(true);

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
}
#pragma endregion

#pragma region editMenu
#include "HMUI/Touchable.hpp"
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

void setButtons(QuestUI::IncrementSetting* inc) {
    auto arr = inc->get_gameObject()->get_transform()->GetChild(1)->GetComponentsInChildren<UnityEngine::UI::Button*>();
    auto decButton = arr->get(0);
    auto incButton = arr->get(1);
    decButton->set_interactable(inc->CurrentValue != inc->MinValue);
    incButton->set_interactable(inc->CurrentValue != inc->MaxValue);
}
void EditMenu::init(DefaultCube* parent) {
    getLogger().info("menu init");
    auto background = get_gameObject()->AddComponent<QuestUI::Backgroundable*>();
    background->ApplyBackgroundWithAlpha(il2cpp_utils::createcsstr("round-rect-panel"), 0.5);
    background->background->set_raycastTarget(true);

    get_gameObject()->AddComponent<HMUI::Touchable*>();
    get_transform()->SetParent(parent->get_transform());
    get_transform()->set_localPosition({0, 0, 0});
    float inv_size = 0.03/parent->size;
    get_transform()->set_localScale({inv_size, inv_size, inv_size});
    // doesn't resize to contents on its own
    reinterpret_cast<UnityEngine::RectTransform*>(get_transform())->set_sizeDelta({75, 25});
    get_gameObject()->set_active(false);

    // value settings
    valVertical = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
    valVertical->get_rectTransform()->set_anchoredPosition({-4, -2});
    valVertical->set_childControlWidth(true);
    valVertical->set_childControlHeight(true);
    valVertical->set_childForceExpandHeight(false);

    // lines at the end set the widths to 60
    typeInc = QuestUI::BeatSaberUI::CreateIncrementSetting(valVertical->get_transform(), "Cube Type", 0, 1, parent->type, 0, 2, [parent, this](int value){
        parent->setType(value);
        Cubes.SetCubeValue(parent->index, CubeInfo(parent));
        this->typeInc->Text->SetText(il2cpp_utils::createcsstr(cubeTypes[value]));
        setButtons(this->typeInc);
    });
    setButtons(typeInc);
    typeInc->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
    typeInc->Text->SetText(il2cpp_utils::createcsstr(cubeTypes[parent->type]));

    eventInc = QuestUI::BeatSaberUI::CreateIncrementSetting(valVertical->get_transform(), "Hit Action", 0, 1, parent->hitAction, 0, 4, [parent, this](int value){
        parent->hitAction = value;
        Cubes.SetCubeValue(parent->index, CubeInfo(parent));
        this->eventInc->Text->SetText(il2cpp_utils::createcsstr(cutEvents[value]));
        setButtons(this->eventInc);
    });
    setButtons(eventInc);
    eventInc->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
    eventInc->Text->SetText(il2cpp_utils::createcsstr(cutEvents[parent->hitAction]));

    QuestUI::BeatSaberUI::CreateSliderSetting(valVertical->get_transform(), "Qube Size", 0.01, parent->size, 0.25, 1.5, 0, [parent](float value){
        parent->setSize(value);
    })->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);

    // color settings
    colVertical = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
    colVertical->get_rectTransform()->set_anchoredPosition({-4, -2});
    colVertical->set_childControlWidth(true);
    colVertical->set_childControlHeight(true);
    colVertical->set_childForceExpandHeight(false);
    
    auto colslider = QuestUI::BeatSaberUI::CreateSliderSetting(colVertical->get_transform(), "R", 1, parent->color.r * 255, 0, 255, 0, [parent](float value){
        parent->setColor({value/255, parent->color.g, parent->color.b, 1});
        Cubes.SetCubeValue(parent->index, CubeInfo(parent));
    });
    colslider->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
    reinterpret_cast<UnityEngine::RectTransform*>(colslider->slider->get_transform())->set_sizeDelta({56, 0});

    colslider = QuestUI::BeatSaberUI::CreateSliderSetting(colVertical->get_transform(), "G", 1, parent->color.g * 255, 0, 255, 0, [parent](float value){
        parent->setColor({parent->color.r, value/255, parent->color.b, 1});
        Cubes.SetCubeValue(parent->index, CubeInfo(parent));
    });
    colslider->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
    reinterpret_cast<UnityEngine::RectTransform*>(colslider->slider->get_transform())->set_sizeDelta({56, 0});

    colslider = QuestUI::BeatSaberUI::CreateSliderSetting(colVertical->get_transform(), "B", 1, parent->color.b * 255, 0, 255, 0, [parent](float value){
        parent->setColor({parent->color.r, parent->color.g, value/255, 1});
        Cubes.SetCubeValue(parent->index, CubeInfo(parent));
    });
    colslider->get_transform()->get_parent()->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
    reinterpret_cast<UnityEngine::RectTransform*>(colslider->slider->get_transform())->set_sizeDelta({56, 0});
    
    colVertical->get_gameObject()->set_active(false);

    // viewed settings change and lock/close buttons
    auto closeSprite = QuestUI::BeatSaberUI::FileToSprite(getDataDir(modInfo) + "close.png");
    closeButton = QuestUI::BeatSaberUI::CreateUIButton(get_transform(), "", [this](){
        this->get_gameObject()->set_active(false);
    });
    QuestUI::BeatSaberUI::SetButtonSprites(closeButton, closeSprite, closeSprite);
    // closeButton->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_margin({-16.5, 0, 0, 0});
    auto rect = reinterpret_cast<UnityEngine::RectTransform*>(closeButton->get_transform());
    rect->set_anchoredPosition({32, 7});
    rect->set_sizeDelta({8, 8});

    auto col_button = QuestUI::BeatSaberUI::CreateUIButton(get_transform(), "", "ColorPickerButtonSecondary", [this](){
        this->valVertical->get_gameObject()->set_active(!this->valVertical->get_gameObject()->get_active());
        this->colVertical->get_gameObject()->set_active(!this->colVertical->get_gameObject()->get_active());
    });
    rect = reinterpret_cast<UnityEngine::RectTransform*>(col_button->get_transform());
    rect->set_anchoredPosition({32, 0});
    // rect->set_sizeDelta({8, 8});

    colButtonController = col_button->GetComponent<GlobalNamespace::ColorPickerButtonController*>();
    colButtonController->SetColor(parent->color);
    
    auto lockSprite = QuestUI::BeatSaberUI::FileToSprite(getDataDir(modInfo) + "lock.png");
    auto unlockSprite = QuestUI::BeatSaberUI::FileToSprite(getDataDir(modInfo) + "unlock.png");
    lockButton = QuestUI::BeatSaberUI::CreateUIButton(get_transform(), "", [this, parent, lockSprite, unlockSprite](){
        parent->locked = !parent->locked;
        if(parent->locked)
            QuestUI::BeatSaberUI::SetButtonSprites(this->lockButton, lockSprite, lockSprite);
        else
            QuestUI::BeatSaberUI::SetButtonSprites(this->lockButton, unlockSprite, unlockSprite);
    });
    QuestUI::BeatSaberUI::SetButtonSprites(lockButton, unlockSprite, unlockSprite);
    // closeButton->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_margin({-16.5, 0, 0, 0});
    rect = reinterpret_cast<UnityEngine::RectTransform*>(lockButton->get_transform());
    rect->set_anchoredPosition({32, -5});
    rect->set_sizeDelta({8, 8});
}
#pragma endregion