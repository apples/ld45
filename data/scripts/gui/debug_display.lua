local vdom = require('vdom')
local linq = require('linq')
local debug_table = require('gui.debug_table')

return function()
    return vdom.create_element(
        'panel',
        {
            halign='left',
            valign='top',
        },
        vdom.create_element(debug_table, { strings = 'debug_strings', top = 12, left = 0 }),
        vdom.create_element(debug_table, { strings = 'debug_vals', top = 12, right = 250 })
    )
end
