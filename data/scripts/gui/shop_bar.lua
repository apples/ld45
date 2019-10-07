local vdom = require('vdom')

local function item_box(props)
    local function on_click()
        if game_state.money >= props.item.cost then
            game_state.money = game_state.money - props.item.cost
            props.item.action()
        end
    end

    return vdom.create_element('panel', { left = props.left, width = 48, height = 48, texture = 'item_box' },
        vdom.create_element('panel', { halign = 'center', valign = 'center', width = 32, height = 32, texture = props.item.name }),
        vdom.create_element('label', { left = 2, bottom = 2, height = 12, color = 'white', text = '$'..props.item.cost }),
        vdom.create_element('widget', { width = '100%', height = '100%', on_click = on_click })
    )
end

local function cash_counter(props)
    return vdom.create_element('panel', { right = 800, width = 64, height = 24, texture = 'item_box' },
        vdom.create_element('label', { left = 2, bottom = 2, height = 20, color = 'white', text = '$'..props.value })
    )
end

return function(props)
    local context = vdom.useContext()

    local item_boxes = {}

    for i,item in ipairs(context.game_state.shop_items) do
        item_boxes[#item_boxes + 1] = vdom.create_element(item_box, { item = item, left = i * 48 })
    end

    return vdom.create_element('widget', { width = '100%', height = 48 },
        item_boxes,
        vdom.create_element(cash_counter, { value = context.game_state.money })
    )
end

