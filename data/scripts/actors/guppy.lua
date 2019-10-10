local engine = require('engine')
local spawn_bubble = require('archetypes.bubble')
local visitor = require('visitor')

local guppy = {}

function guppy.update(eid, dt)
    local state = engine.entities:get_component(eid, component.script).state
    local spr = engine.entities:get_component(eid, component.sprite)
    local pos
    local vel

    state.next_decision = state.next_decision - dt
    state.next_bubble = state.next_bubble - dt
    state.food = state.food - dt / 2

    if not state.grown then
        state.growth = state.growth + dt
        if state.growth >= state.growthrate then
            state.grown = true
            for _,v in ipairs(spr.frames) do
                v.r = v.r + 1
            end
        end
    end

    if state.food < 0 then
        engine.entities:destroy_entity(eid)
        return
    end

    if state.next_decision <= 0 then
        vel = vel or engine.entities:get_component(eid, component.velocity).vel
        local vx = (math.random() * 2 - 1) * 5
        local vy = (math.random() * 2 - 1) * 3

        vel.x = vx
        vel.y = vy

        local spd = vx*vx + vy*vy

        state.next_decision = math.random() * 3 + 0.5

        spr.speed = spd
    end

    if state.next_bubble <= 0 then
        pos = pos or engine.entities:get_component(eid, component.position).pos
        spawn_bubble({ x = pos.x, y = pos.y })
        play_sfx('bubble')

        state.next_bubble = (math.random() * 20 + state.bubblemin) / (state.grown and state.grownbubble or 1)
    end

    local fooding = false

    if state.food < 20 then
        pos = pos or engine.entities:get_component(eid, component.position).pos
        local closest = nil
        visitor.visit(
            {component.food, component.position},
            function (eid, food, foodpos)
                local dist = math.abs(foodpos.pos.x - pos.x)
                if closest == nil or dist < closest.dist then
                    closest = {
                        eid = eid,
                        food = food,
                        pos = foodpos.pos,
                        dist = dist
                    }
                end
            end
        )
        if closest then
            vel = vel or engine.entities:get_component(eid, component.velocity).vel
            local xdist = closest.pos.x - pos.x
            local ydist = closest.pos.y - pos.y
            local dist = math.sqrt(xdist*xdist + ydist*ydist)
            xdist = xdist / dist
            ydist = ydist / dist
            vel.x = xdist * 4
            vel.y = ydist * 4
            fooding = true
        end
    end

    if state.food < 5 and not fooding then
        spr.flip = not spr.flip
    else
        vel = vel or engine.entities:get_component(eid, component.velocity).vel
        spr.flip = vel.x < 0
    end
end

function guppy.on_collide(me, other, region)
    local state = engine.entities:get_component(me.eid, component.script).state
    if state.food < 20 and engine.entities:has_component(other.eid, component.food) then
        local food = engine.entities:get_component(other.eid, component.food)
        local foodstate = engine.entities:get_component(other.eid, component.script).state
        if not foodstate.deleteme then
            state.food = state.food + food.amount
            foodstate.deleteme = true
        end
    end
end

return guppy

