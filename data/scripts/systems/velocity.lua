local engine = require('engine')

local velocity = {}

function velocity.visit(dt)
    engine.velocity_system(engine.entities, dt)
end

return velocity
