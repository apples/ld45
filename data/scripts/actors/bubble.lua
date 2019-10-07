local engine = require('engine')

local bubble = {}

function bubble.update(eid, dt)
    game_state.money = game_state.money + 1
    engine.entities:destroy_entity(eid)
end

return bubble

