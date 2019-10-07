local engine = require('engine')

return function(init)
    init = init or {}

    local ent = engine.entities:create_entity()

    local position = component.position.new()
    position.pos.x = init.x or 0
    position.pos.y = init.y or 0

    local velocity = component.velocity.new()

    local f = component.rowcol.new
    local r = init.sprite or 0
    local sprite = component.sprite.new()
    sprite.frames = { f(r, 0), f(r, 1), f(r, 2) }
    sprite.speed = 3
    sprite.bounce = true

    local body = component.body.new()
    body.width = 0.5
    body.height = 0.5

    local script = component.script.new()
    script.next_tick = 0
    script.name = 'guppy'
    script.state = {
        next_decision = 1,
        next_bubble = 15,
        food = 30,
        growth = 0,
        grown = false,
        grownbubble = init.grownbubble,
        growthrate = init.growthrate,
        bubblemin = init.bubblemin
    }

    engine.entities:add_component(ent, position)
    engine.entities:add_component(ent, velocity)
    engine.entities:add_component(ent, sprite)
    engine.entities:add_component(ent, body)
    engine.entities:add_component(ent, script)

    return ent
end
