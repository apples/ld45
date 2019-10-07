local engine = require('engine')
local visitor = require('visitor')
local linq = require('linq')

local function call_script(a, b, region)
    if engine.entities:has_component(a.eid, component.script) then
        local script = engine.entities:get_component(a.eid, component.script)
        local script_impl = require('actors.' .. script.name)

        if script_impl.on_collide ~= nil then
            script_impl.on_collide(a, b, region)
        end
    end
end

local function with_vel(eid, func)
    if engine.entities:has_component(eid, component.velocity) then
        local vel = engine.entities:get_component(eid, component.velocity).vel
        func(vel)
    end
end

local function resolve_collision(a, b, region)
    local r1 = call_script(a, b, region)

    if r1 == 'abort' then return 'abort' end

    local r2 = call_script(b, a, region)

    if r2 == 'abort' then return 'abort' end

    local w = region.right - region.left
    local h = region.top - region.bottom

    if a.body.solid and b.body.solid then
        local xresponse = 1
        local yresponse = 1

        if w < h then xresponse = 2 else yresponse = 2 end

        if not (
            b.left < a.left and a.left < b.right and a.body.edge_left and b.body.edge_right or
            a.left < b.left and b.left < a.right and b.body.edge_left and a.body.edge_right or
            b.left < a.right and a.right < b.right and a.body.edge_right and b.body.edge_left or
            a.left < b.right and b.right < a.right and b.body.edge_right and a.body.edge_left
        )
        then
            xresponse = 0
        end

        if not (
            b.bottom < a.bottom and a.bottom < b.top and a.body.edge_bottom and b.body.edge_top or
            a.bottom < b.bottom and b.bottom < a.top and b.body.edge_bottom and a.body.edge_top or
            b.bottom < a.top and a.top < b.top and a.body.edge_top and b.body.edge_bottom or
            a.bottom < b.top and b.top < a.top and b.body.edge_top and a.body.edge_bottom
        )
        then
            yresponse = 0
        end

        if xresponse == 0 and yresponse == 0 then return end

        if a.body.dynamic then
            if xresponse > yresponse then
                with_vel(a.eid, function (vel) vel.x = 0 end)

                if b.body.dynamic then
                    with_vel(b.eid, function (vel) vel.x = 0 end)

                    if a.position.pos.x < b.position.pos.x then
                        a.position.pos.x = a.position.pos.x - w / 2
                        b.position.pos.x = b.position.pos.x + w / 2
                    else
                        a.position.pos.x = a.position.pos.x + w / 2
                        b.position.pos.x = b.position.pos.x - w / 2
                    end
                else
                    if a.position.pos.x < b.position.pos.x then
                        a.position.pos.x = a.position.pos.x - w
                    else
                        a.position.pos.x = a.position.pos.x + w
                    end
                end
            else
                with_vel(a.eid, function (vel) vel.y = 0 end)

                if b.body.dynamic then
                    with_vel(b.eid, function (vel) vel.y = 0 end)

                    if a.position.pos.y < b.position.pos.y then
                        a.position.pos.y = a.position.pos.y - h / 2
                        b.position.pos.y = b.position.pos.y + h / 2
                    else
                        a.position.pos.y = a.position.pos.y + h / 2
                        b.position.pos.y = b.position.pos.y - h / 2
                    end
                else
                    if a.position.pos.y < b.position.pos.y then
                        a.position.pos.y = a.position.pos.y - h
                    else
                        a.position.pos.y = a.position.pos.y + h
                    end
                end
            end
        elseif b.body.dynamic then
            if xresponse > yresponse then
                with_vel(b.eid, function (vel) vel.x = 0 end)

                if b.position.pos.x < a.position.pos.x then
                    b.position.pos.x = b.position.pos.x - w
                else
                    b.position.pos.x = b.position.pos.x + w
                end
            else
                with_vel(b.eid, function (vel) vel.y = 0 end)

                if b.position.pos.y < a.position.pos.y then
                    b.position.pos.y = b.position.pos.y - h
                else
                    b.position.pos.y = b.position.pos.y + h
                end
            end
        end
    end
end

local function update_body(bod)
    bod.left = bod.position.pos.x - bod.body.width / 2
    bod.right = bod.position.pos.x + bod.body.width / 2
    bod.bottom = bod.position.pos.y - bod.body.height / 2
    bod.top = bod.position.pos.y + bod.body.height / 2
end

local physics = {}

function physics.visit(dt)
    trace_push('physics.visit')

    local bodies = {}

    trace_push('physics.visit[gather bodies]')
    visitor.visit(
        {component.position, component.body},
        function (eid, position, body)
            local pos = position.pos
            local x = pos.x
            local y = pos.y
            local w = body.width / 2
            local h = body.height / 2
            bodies[#bodies + 1] = {
                eid = eid,
                position = position,
                body = body,
                left = x - w,
                right = x + w,
                bottom = y - h,
                top = y + h
            }
        end
    )
    trace_pop('physics.visit[gather bodies]')

    trace_push('physics.visit[sort bodies]')
    table.sort(bodies, function (a, b) return a.left < b.left end)
    trace_pop('physics.visit[sort bodies]')

    local axis_list = {}

    trace_push('physics.visit[sweep]')
    for _,a in ipairs(bodies) do
        axis_list = linq(axis_list)
            :where(function (b) return a.left < b.right end)
            :tolist()

        for _,b in ipairs(axis_list) do
            local region = {
                left = math.max(a.left, b.left),
                right = math.min(a.right, b.right),
                bottom = math.max(a.bottom, b.bottom),
                top = math.min(a.top, b.top),
            }

            local mx = region.right - region.left
            local my = region.top - region.bottom

            local threshold = 1/32

            if mx > 0 and my > 0 and (mx > threshold or my > threshold) then
                trace_push('physics.visit[resolve_collision]')
                if resolve_collision(a, b, region) == 'abort' then goto abort end
                trace_pop('physics.visit[resolve_collision]')
                update_body(a)
                update_body(b)
            end
        end

        axis_list[#axis_list + 1] = a
    end
    trace_pop('physics.visit[sweep]')

    trace_push('physics.visit[border]')
    do
        local scrw = 25/2
        local scrh = 17.25/2
        for _,bod in ipairs(bodies) do
            local pos = bod.position.pos
            local left = bod.left
            local right = bod.right
            local bottom = bod.bottom
            local top = bod.top

            if left < -scrw then pos.x = pos.x + -scrw - left end
            if right > scrw then pos.x = pos.x + scrw - right end
            if bottom < -scrh then pos.y = pos.y + -scrh - bottom end
            if top > scrh then pos.y = pos.y + scrh - top end
        end
    end
    trace_pop('physics.visit[border]')

    trace_pop('physics.visit')
    do return end

    ::abort::
    trace_pop('physics.visit[resolve_collision]')
    trace_pop('physics.visit[sweep]')
    trace_pop('physics.visit')
end

return physics
