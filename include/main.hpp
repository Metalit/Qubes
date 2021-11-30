#pragma once

#include "cube.hpp"

#include "modloader/shared/modloader.hpp"

#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "extern/custom-types/shared/types.hpp"

#include "UnityEngine/Resources.hpp"

Logger& getLogger();

#include "GlobalNamespace/NoteDebris.hpp"
#include "UnityEngine/Color.hpp"
#include "VRUIControls/VRPointer.hpp"
#include "GlobalNamespace/HapticFeedbackController.hpp"
#include "GlobalNamespace/PauseController.hpp"
#include "HMUI/ViewController.hpp"

extern ModInfo modInfo;

Cube* makeCube(CubeInfo info, QubesConfig& config, int index);
DefaultCube* makeDefaultCube(CubeInfo info, QubesConfig& config, int index);

// extern std::vector<QubesConfig> QubesConfigs; in modconfig.hpp
extern DefaultCube* defaultCube;

extern std::vector<Cube*> cubeArr;
extern GlobalNamespace::NoteDebris* debrisPrefab;
extern UnityEngine::Color lastColor;
extern VRUIControls::VRPointer* pointer;
extern GlobalNamespace::HapticFeedbackController* haptics;
extern GlobalNamespace::PauseController* pauser;

extern bool inMenu, inGameplay;