#include "velocity_system.hpp"

#include "components.hpp"

void velocity_system(ember_database& entities, float dt, sol::this_state s) {
    entities.visit([&](const component::velocity& velocity, component::position& position){
        position.pos += velocity.vel * dt;
    });
}
