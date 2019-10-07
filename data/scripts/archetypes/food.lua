local engine = require('engine')

return function(init)
    init = init or {}

    local ent = engine.entities:create_entity()

    local position = component.position.new()
    position.pos.x = init.x or 0
    position.pos.y = init.y or 17.25/2

    local velocity = component.velocity.new()
    velocity.vel.y = -1

    local f = component.rowcol.new
    local sprite = component.sprite.new()
    sprite.frames = { f(1, 3), f(1, 4) }
    sprite.speed = 2
    sprite.bounce = true

    local body = component.body.new()
    body.width = 0.25
    body.height = 0.25

    local script = component.script.new()
    script.next_tick = 0
    script.name = 'food'
    script.state = {
        deleteme = false
    }

    local food = component.food.new()
    food.amount = init.amount or 10

    engine.entities:add_component(ent, position)
    engine.entities:add_component(ent, velocity)
    engine.entities:add_component(ent, sprite)
    engine.entities:add_component(ent, body)
    engine.entities:add_component(ent, script)
    engine.entities:add_component(ent, food)

    return ent
end
