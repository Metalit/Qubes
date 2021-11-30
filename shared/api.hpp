#pragma once

#include "UnityEngine/GameObject.hpp"
#include "conditional-dependencies/shared/main.hpp"

namespace Qubes {
    // configs for each mod must be registered for them to work independently of the regular behaviour
    inline bool RegisterConfig(std::string modName) {
        static auto func = CondDeps::Find<void, std::string>("qubes", "RegisterConfig");
        if(func)
            func.value()(modName);
        return (bool)func;
    }

    inline std::optional<int> ConfigSize(std::string modName) {
        static auto func = CondDeps::Find<int, std::string>("qubes", "ConfigSize");
        if(func)
            return func.value()(modName);
        return std::nullopt;
    }
    
    inline bool SaveCube(UnityEngine::GameObject* cube, std::string modName) {
        static auto func = CondDeps::Find<void, UnityEngine::GameObject*, std::string>("qubes", "SaveCube");
        if(func)
            func.value()(cube, modName);
        return (bool)func;
    }

    inline bool DeleteCube(UnityEngine::GameObject* cube, std::string modName) {
        static auto func = CondDeps::Find<void, UnityEngine::GameObject*, std::string>("qubes", "DeleteCube");
        if(func)
            func.value()(cube, modName);
        return (bool)func;
    }

    // add a cube to the config if there is not one already, then create and return
    inline std::optional<UnityEngine::GameObject*> CreateCube(std::string modName, int index) {
        static auto func = CondDeps::Find<UnityEngine::GameObject*, std::string, int>("qubes", "CreateCube");
        if(func)
            return func.value()(modName, index);
        return std::nullopt;
    }
}

// feel free to ask for more api features or qube class methods