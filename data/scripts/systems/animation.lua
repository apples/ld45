local engine = require('engine')
local visitor = require('visitor')

local G = -100

local gravity = {}

function gravity.visit(dt)
    visitor.visit(
        {component.sprite},
        function (eid, sprite)
            if sprite.speed ~= 0 and #sprite.frames > 1 then
                sprite.timer = sprite.timer + sprite.speed * dt
                if sprite.timer >= 1 then
                    sprite.timer = sprite.timer - 1
                    if sprite.bounce then
                        if sprite.dir == 1 and sprite.cur_frame == #sprite.frames - 1 then
                            sprite.dir = -1
                        elseif sprite.dir == -1 and sprite.cur_frame == 0 then
                            sprite.dir = 1
                        end
                    end
                    if sprite.loop and sprite.cur_frame == #sprite.frames - 1 then
                        sprite.cur_frame = 0
                    else
                        sprite.cur_frame = sprite.cur_frame + sprite.dir
                    end
                end
            end
        end
    )
end

return gravity
