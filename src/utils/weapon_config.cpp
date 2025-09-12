//
// Created by rain on 8/1/25.
//

#include "weapon_config.h"

// Define the static arrays
float WeaponConfig::s_WeaponDamage[] = {
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    82.5f, 0.0f, 1.0f, 9.9f, 46.2f, 82.5f, 8.25f, 13.2f,
    46.2f, 3.3f, 3.3f, 4.95f, 6.6f, 8.25f, 9.9f, 9.9f,
    6.6f, 24.75f, 41.25f, 82.5f, 82.5f, 1.0f, 46.2f, 82.5f,
    0.0f, 0.33f, 0.33f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    2.64f, 9.9f, 330.0f, 82.5f, 1.0f, 1.0f, 165.0f
};

int WeaponConfig::s_MaxWeaponShootRate[] = {
    250, 250, 250, 250, 250, 250, 250, 250,
    250, 30, 250, 250, 250, 250, 250, 250,
    0, 0, 0, 90, 20, 0, 160, 120,
    120, 800, 120, 120, 50, 90, 90, 90,
    90, 50, 800, 800, 0, 0, 0, 20,
    0, 0, 10, 10, 0, 0, 0, 0,
    0, 400
};

float WeaponConfig::s_WeaponRange[] = {
    1.76f, 1.76f, 1.76f, 1.76f, 1.76f, 1.76f, 1.6f, 1.76f,
    1.76f, 1.76f, 1.76f, 1.76f, 1.76f, 1.76f, 1.76f, 1.76f,
    40.0f, 40.0f, 40.0f, 90.0f, 75.0f, 0.0f, 35.0f, 35.0f,
    35.0f, 40.0f, 35.0f, 40.0f, 35.0f, 45.0f, 70.0f, 90.0f,
    35.0f, 100.0f, 320.0f, 55.0f, 55.0f, 5.1f, 75.0f, 40.0f,
    25.0f, 6.1f, 10.1f, 100.0f, 100.0f, 100.0f, 1.76f
};

// Implementation of the methods
float WeaponConfig::GetWeaponDamage(int id) {
    if (id < 0 || id >= sizeof(s_WeaponDamage) / sizeof(s_WeaponDamage[0])) {
        return -1.0f; // Invalid ID
    }
    return s_WeaponDamage[id];
}

int WeaponConfig::GetMaxWeaponShootRate(int id) {
    if (id < 0 || id >= sizeof(s_MaxWeaponShootRate) / sizeof(s_MaxWeaponShootRate[0])) {
        return -1; // Invalid ID
    }
    return s_MaxWeaponShootRate[id];
}

float WeaponConfig::GetWeaponRange(int id) {
    if (id < 0 || id >= sizeof(s_WeaponRange) / sizeof(s_WeaponRange[0])) {
        return -1.0f; // Invalid ID
    }
    return s_WeaponRange[id];
}

std::string WeaponConfig::GetWeaponName(int id) {
    switch (id) {
        case 0: return "Fist";
        case 1: return "Brass Knuckles";
        case 2: return "Golf Club";
        case 3: return "Nightstick";
        case 4: return "Knife";
        case 5: return "Baseball Bat";
        case 6: return "Shovel";
        case 7: return "Pool Cue";
        case 8: return "Katana";
        case 9: return "Chainsaw";
        case 10: return "Purple Dildo";
        case 11: return "Dildo";
        case 12: return "Vibrator";
        case 13: return "Silver Vibrator";
        case 14: return "Flowers";
        case 15: return "Cane";
        case 16: return "Grenade";
        case 17: return "Tear Gas";
        case 18: return "Molotov";
        case 22: return "Colt 45";
        case 23: return "Silenced 9mm";
        case 24: return "Desert Eagle";
        case 25: return "Shotgun";
        case 26: return "Sawn-Off";
        case 27: return "Combat Shotgun";
        case 28: return "Micro Uzi";
        case 29: return "MP5";
        case 30: return "AK-47";
        case 31: return "M4";
        case 32: return "Tec-9";
        case 33: return "Country Rifle";
        case 34: return "Sniper Rifle";
        case 35: return "Rocket Launcher";
        case 36: return "Heat-Seeking Rocket Launcher";
        case 37: return "Flamethrower";
        case 38: return "Minigun";
        case 39: return "Satchel Charge";
        case 40: return "Detonator";
        case 41: return "Spray Can";
        case 42: return "Fire Extinguisher";
        case 43: return "Camera";
        case 44: return "Night Vision Goggles";
        case 45: return "Thermal Goggles";
        case 46: return "Parachute";
        case 47: return "Tec-9 (dual)";
        default: return "Unknown";
    }
}
