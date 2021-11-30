#include "main.hpp"

#include "conditional-dependencies/shared/main.hpp"

// warning spam
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic push

Qubes::QubesConfig& findConfig(std::string modName) {
    for(auto& cfg : QubesConfigs) {
        if(cfg.name == modName)
            return cfg;
    }
    getLogger().info("No config found with name %s", modName.c_str());
    CRASH_UNLESS(false);
}

EXPOSE_API(RegisterConfig, void, std::string modName) {
    getLogger().info("Registering config: %s", modName.c_str());
    QubesConfigs.push_back(Qubes::QubesConfig(modName));
}

EXPOSE_API(ConfigSize, int, std::string modName) {
    return findConfig(modName).cubes.size();
}

EXPOSE_API(SaveCube, void, UnityEngine::GameObject* ob, std::string modName) {
    // find config
    auto& config = findConfig(modName);
    // get cube object
    Qubes::Cube* cube;
    if(ob->TryGetComponent<Qubes::Cube*>(byref(cube)))
        config.SetCubeValue(cube->index, Qubes::CubeInfo(cube));
}

EXPOSE_API(DeleteCube, void, UnityEngine::GameObject* ob, std::string modName) {
    // find config
    auto& config = findConfig(modName);
    // get cube object
    Qubes::Cube* cube;
    if(ob->TryGetComponent<Qubes::Cube*>(byref(cube)))
        config.RemoveCube(cube->index);
}

EXPOSE_API(CreateCube, UnityEngine::GameObject*, std::string modName, int index) {
    // find config
    auto& config = findConfig(modName);
    // default cube might not be made yet (but we want to use it if it is)
    Qubes::CubeInfo defInfo = defaultCube ? CubeInfo(defaultCube) : QubesConfigs[1].cubes[0];
    // add a new cube?
    if(index >= config.cubes.size() || index < 0) {
        index = config.cubes.size();
        config.AddCube(defInfo);
    }
    // make cube, add to arr if needed
    auto newCubeInfo = config.cubes[index];
    auto madeCube = makeCube(newCubeInfo, config, index);
    if(modName == "qubes") {
        cubeArr.push_back(madeCube);
    }
    return madeCube->get_gameObject();
}