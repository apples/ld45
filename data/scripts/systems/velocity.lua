local engine = require('engine')
local visitor = require('visitor')
local linq = require('linq')

local velocity = {}

function velocity.visit(dt)
    visitor.visit(
        {component.velocity, component.position},
        function (eid, velocity, position)
            position.pos.x = position.pos.x + velocity.vel.x * dt
            position.pos.y = position.pos.y + velocity.vel.y * dt
        end
    )
end

return velocity
