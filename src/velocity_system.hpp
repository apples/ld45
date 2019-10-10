#pragma once

#include "entities.hpp"

#include <sol.hpp>

void velocity_system(ember_database& entities, float dt, sol::this_state s);
