local scripting = require('systems.scripting')
local velocity = require('systems.velocity')
local physics = require('systems.physics')
local animation = require('systems.animation')

local systems = {}

local function run(name, sys, delta)
    trace_push('(SYSTEM) '..name)
    sys.visit(delta)
    trace_pop('(SYSTEM) '..name)
end

function systems.visit(delta)
    run('scripting', scripting, delta)
    run('velocity', velocity, delta)
    run('physics', physics, delta)
    run('animation', animation, delta)
end

return systems

