local scripting = require('systems.scripting')
local velocity = require('systems.velocity')
local physics = require('systems.physics')
local animation = require('systems.animation')

local systems = {}

function systems.visit(delta)
    scripting.visit(delta)
    velocity.visit(delta)
    physics.visit(delta)
    animation.visit(delta)
end

return systems

