#include "main.hpp"

#pragma region macros
#define SetArr(name) auto name##_arr = obj[#name].GetArray(); \
for(int i = 0; i < name##_arr.Size(); i++) { \
    name[i] = name##_arr[i].GetFloat(); \
}
#define JSONArr(name) rapidjson::Value name##_arr(rapidjson::kArrayType); \
for(auto i : name) { \
    name##_arr.PushBack(i, allocator); \
} \
v.AddMember(#name, name##_arr, allocator)
#define valArr3(name, value, v0, v1, v2) auto name##_v = value; name[0] = name##_v.v0; name[1] = name##_v.v1; name[2] = name##_v.v2
#define valArr4(name, value, v0, v1, v2, v3) auto name##_v = value; name[0] = name##_v.v0; name[1] = name##_v.v1; name[2] = name##_v.v2, name[3] = name##_v.v3
#pragma endregion

CubeInfo::CubeInfo(UnityEngine::Vector3 in_pos, UnityEngine::Quaternion in_rot, UnityEngine::Color in_color, int in_type, int in_hitAction, float in_size, bool in_locked) {
    valArr3(pos, in_pos, x, y, z);
    valArr4(rot, in_rot, x, y, z, w);
    valArr4(color, in_color, r, g, b, a);
    type = in_type;
    hitAction = in_hitAction;
    size = in_size;
    locked = in_locked;
}

CubeInfo::CubeInfo(rapidjson::Value& obj) {
    // reads values from given "subconfig"
    SetArr(pos);
    SetArr(rot);
    SetArr(color);
    type = obj["type"].GetInt();
    hitAction = obj["hitAction"].GetInt();
    size = obj["size"].GetFloat();
    locked = obj["locked"].GetBool();
}

CubeInfo::CubeInfo(Qubes::DefaultCube* cube) {
    // get values from cube
    valArr3(pos, cube->get_transform()->get_position(), x, y, z);
    valArr4(rot, cube->get_transform()->get_rotation(), x, y, z, w);
    valArr4(color, cube->color, r, g, b, a);
    type = cube->type;
    hitAction = cube->hitAction;
    size = cube->size;
    locked = cube->locked;
}

rapidjson::Value CubeInfo::ToJSON(rapidjson::Document::AllocatorType& allocator) {
    // returns a json object with its info
    rapidjson::Value v(rapidjson::kObjectType);
    JSONArr(pos);
    JSONArr(rot);
    JSONArr(color);
    v.AddMember("type", type, allocator);
    v.AddMember("hitAction", hitAction, allocator);
    v.AddMember("size", size, allocator);
    v.AddMember("locked", locked, allocator);
    return v;
}

// all these assume config is in correct format
void QubesConfig::Init(Configuration* cfg) {
    // store config, important if we want to ever use it
    config = cfg;
    if(config->config.HasMember("cubes")) {
        getLogger().info("Loading cubes from config");
        LoadValue();
    } else {
        getLogger().info("Creating cubes in config");
        auto& allocator = config->config.GetAllocator();
        config->config.AddMember("cubes", rapidjson::Value(rapidjson::kArrayType), allocator);
        // add default cube
        auto defInfo = CubeInfo({-3.7, 1, 1.2}, UnityEngine::Quaternion::Euler({0, -60, 0}), {0.5, 0.5, 0.5, 1}, 2, 0, 1, false);
        config->config["cubes"].GetArray().PushBack(defInfo.ToJSON(allocator), allocator);
        config->Write();
        cubes.push_back(defInfo);
    }
}

void QubesConfig::LoadValue() {
    auto section = config->config["cubes"].GetArray();
    for(int i = 0; i < section.Size(); i++) {
        cubes.push_back(CubeInfo(section[i]));
    }
}

void QubesConfig::SetValue() {
    // deletes and rewrites cube section
    auto& allocator = config->config.GetAllocator();
    auto section = config->config["cubes"].GetArray();
    section.Clear();
    for(auto cube : cubes) {
        section.PushBack(cube.ToJSON(allocator), allocator);
    }
    config->Write();
}

void QubesConfig::AddCube(CubeInfo cube) {
    auto& allocator = config->config.GetAllocator();
    auto section = config->config["cubes"].GetArray();
    section.PushBack(cube.ToJSON(allocator), allocator);
    config->Write();
}

void QubesConfig::SetCubeValue(int index, CubeInfo value) {
    auto& allocator = config->config.GetAllocator();
    auto section = config->config["cubes"].GetArray();
    section[index] = value.ToJSON(allocator);
    config->Write();
}

void QubesConfig::RemoveCube(int index) {
    // needs indices of later cubes to be updated
    auto section = config->config["cubes"].GetArray();
    // can I have erase by index please
    auto iter = section.Begin();
    for(int i = 0; i < index; i++) {
        ++iter;
    }
    section.Erase(iter);
    config->Write();
}