local engine = require('engine')

return function(init)
    init = init or {}

    local ent = engine.entities:create_entity()

    local position = component.position.new()
    position.pos.x = init.x or 0
    position.pos.y = init.y or 0

    local velocity = component.velocity.new()
    velocity.vel.x = init.vx or 0
    velocity.vel.y = init.vy or 1

    local f = component.rowcol.new
    local sprite = component.sprite.new()
    sprite.frames = { f(0, 3) }

    local script = component.script.new()
    script.next_tick = 120
    script.name = 'bubble'

    engine.entities:add_component(ent, position)
    engine.entities:add_component(ent, velocity)
    engine.entities:add_component(ent, sprite)
    engine.entities:add_component(ent, script)

    return ent
end
