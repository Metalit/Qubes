#include "main.hpp"

#include "UnityEngine/RectTransform_Axis.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/ViewController_AnimationType.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "questui/shared/BeatSaberUI.hpp"

DEFINE_TYPE(Qubes, GlobalSettings);
DEFINE_TYPE(Qubes, CreationSettings);
DEFINE_TYPE(Qubes, ButtonSettings);
DEFINE_TYPE(Qubes, ModSettings);

using namespace QuestUI;

// button and controller names, need to be synchronized with buttons in main.cpp
const std::vector<std::string> buttonNames = { "Side Trigger", "A/X Button", "B/Y Button", "None" };
const std::vector<std::string> controllerNames = { "Left", "Right", "Both" };

#pragma region flowCoordinator
void Qubes::ModSettings::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    // set active in case show in menu is disabled
    for(auto cube : cubeArr) {
        cube->setActive(true);
    }

    if(!firstActivation)
        return;
    
    if(!globalSettings)
        globalSettings = BeatSaberUI::CreateViewController<Qubes::GlobalSettings*>();
    if(!creationSettings)
        creationSettings = BeatSaberUI::CreateViewController<Qubes::CreationSettings*>();
    if(!buttonSettings)
        buttonSettings = BeatSaberUI::CreateViewController<Qubes::ButtonSettings*>();
    
    showBackButton = true;
    SetTitle(il2cpp_utils::createcsstr("Qube Settings"), HMUI::ViewController::AnimationType::In);

    ProvideInitialViewControllers(globalSettings, creationSettings, buttonSettings, nullptr, nullptr);
}

void Qubes::ModSettings::BackButtonWasPressed(HMUI::ViewController* topViewController) {
    parentFlowCoordinator->DismissFlowCoordinator(this, HMUI::ViewController::AnimationDirection::Horizontal, nullptr, false);
    
    // set back to show in menu value
    for(auto cube : cubeArr) {
        cube->setActive(getModConfig().ShowInMenu.GetValue());
    }
}
#pragma endregion

#pragma region globalSettings
void Qubes::GlobalSettings::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if(!firstActivation)
        return;
    
    get_gameObject()->AddComponent<HMUI::Touchable*>();
    auto vertical = BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
    // prevent the layout from expanding to the full height of its container
    vertical->set_childControlHeight(false);
    vertical->set_childForceExpandHeight(false);
    vertical->set_spacing(1);
    auto verticalTransform = vertical->get_transform();
    
    BeatSaberUI::CreateText(verticalTransform, "Global Settings")->set_alignment(514);

    AddConfigValueToggle(verticalTransform, getModConfig().ShowInMenu);
    AddConfigValueToggle(verticalTransform, getModConfig().ShowInLevel);
    AddConfigValueToggle(verticalTransform, getModConfig().ReqDirection);
    AddConfigValueToggle(verticalTransform, getModConfig().Debris);
    AddConfigValueIncrementFloat(verticalTransform, getModConfig().RespawnTime, 1, 0.5, 0, 5);
    AddConfigValueIncrementFloat(verticalTransform, getModConfig().Vibration, 1, 0.1, 0, 2);
}
#pragma endregion

#pragma region creationSettings
void Qubes::CreationSettings::DidDeactivate(bool removedFromHierarchy, bool screenSystemDisabling) {
    defaultCube->setActive(false);
}

void Qubes::CreationSettings::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    defaultCube->setActive(true);
    defaultCube->setMenuActive(true);

    if(!firstActivation)
        return;
    
    get_gameObject()->AddComponent<HMUI::Touchable*>();
    auto vertical = BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
    // prevent the layout from expanding to the full height of its container
    vertical->set_childControlHeight(false);
    vertical->set_childForceExpandHeight(false);
    vertical->set_spacing(1);
    auto verticalTransform = vertical->get_transform();

    BeatSaberUI::CreateText(verticalTransform, "Creation Settings")->set_alignment(514);

    AddConfigValueIncrementFloat(verticalTransform, getModConfig().CreateDist, 1, 0.5, 0, 3);
    AddConfigValueToggle(verticalTransform, getModConfig().CreateRot);

    BeatSaberUI::CreateText(verticalTransform, "Default Cube")->set_alignment(514);
}
#pragma endregion

#pragma region buttonSettings
// makes the paired dropdown menus for controller bindings
void makeDropdowns(UnityEngine::Transform* parent, ConfigUtils::ConfigValue<int>& buttonSetting, ConfigUtils::ConfigValue<int>& controllerSetting) {
    auto layout = BeatSaberUI::CreateHorizontalLayoutGroup(parent)->get_transform();
    auto d = BeatSaberUI::CreateDropdown(layout, buttonSetting.GetName(), buttonNames[buttonSetting.GetValue()], buttonNames, [&buttonSetting](std::string_view value){
        int i = 0;
        for(auto bstr : buttonNames) {
            if(bstr == value.data()) {
                buttonSetting.SetValue(i);
                break;
            }
            i++;
        }
    });
    reinterpret_cast<UnityEngine::RectTransform*>(d->get_transform())->SetSizeWithCurrentAnchors(UnityEngine::RectTransform::Axis::Horizontal, 27);
    auto p = d->get_transform()->get_parent();
    reinterpret_cast<UnityEngine::RectTransform*>(p->Find(il2cpp_utils::createcsstr("Label")))->set_anchorMax({2, 1});
    p->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(48);
    d = BeatSaberUI::CreateDropdown(layout, controllerSetting.GetName(), controllerNames[controllerSetting.GetValue()], controllerNames, [&controllerSetting](std::string_view value){
        int i = 0;
        for(auto cstr : controllerNames) {
            if(cstr == value.data()) {
                controllerSetting.SetValue(i);
                break;
            }
            i++;
        }
    });
    reinterpret_cast<UnityEngine::RectTransform*>(d->get_transform())->SetSizeWithCurrentAnchors(UnityEngine::RectTransform::Axis::Horizontal, 22);
    p = d->get_transform()->get_parent();
    reinterpret_cast<UnityEngine::RectTransform*>(p->Find(il2cpp_utils::createcsstr("Label")))->set_anchorMax({2, 1});
    p->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(42);
}

void Qubes::ButtonSettings::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if(!firstActivation)
        return;

    get_gameObject()->AddComponent<HMUI::Touchable*>();
    auto vertical = BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
    // prevent the layout from expanding to the full height of its container
    vertical->set_childControlHeight(false);
    vertical->set_childForceExpandHeight(false);
    vertical->set_spacing(1);
    auto verticalTransform = vertical->get_transform();

    BeatSaberUI::CreateText(verticalTransform, "Controller Settings")->set_alignment(514);

    makeDropdowns(verticalTransform, getModConfig().BtnMake, getModConfig().CtrlMake);
    makeDropdowns(verticalTransform, getModConfig().BtnEdit, getModConfig().CtrlEdit);
    makeDropdowns(verticalTransform, getModConfig().BtnDel, getModConfig().CtrlDel);
    
    AddConfigValueIncrementFloat(verticalTransform, getModConfig().MoveSpeed, 1, 0.1, 0, 5);
    AddConfigValueIncrementFloat(verticalTransform, getModConfig().RotSpeed, 1, 0.1, 0, 5);
    AddConfigValueToggle(verticalTransform, getModConfig().LeftThumbMove);
}
#pragma endregion