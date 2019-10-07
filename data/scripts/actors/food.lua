local engine = require('engine')

local food = {}

function food.update(eid, dt)
    local pos = engine.entities:get_component(eid, component.position).pos
    local vel = engine.entities:get_component(eid, component.velocity).vel
    local state = engine.entities:get_component(eid, component.script).state

    vel.y = math.max(vel.y - 1 * dt, -1)

    if pos.y < -8.45 or state.deleteme then
        engine.entities:destroy_entity(eid)
    end
end

return food

