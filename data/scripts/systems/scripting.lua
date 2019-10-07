local engine = require('engine')
local visitor = require('visitor')

local scripting = {}

function scripting.visit(dt)
    visitor.visit(
        {component.script},
        function (eid, script)
            if script.next_tick == 0 then
                local script_impl = require('actors.' .. script.name)
                script_impl.update(eid, dt)
            elseif script.next_tick > 0 then
                script.next_tick = script.next_tick - 1
            end
        end
    )
end

return scripting
