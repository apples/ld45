local engine = require('engine')

local physics = {}

function physics.visit(dt)
    engine.physics_system(engine.entities, dt)
end

return physics
