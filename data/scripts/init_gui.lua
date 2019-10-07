local vdom = require('vdom')
local gameplay_gui = require('gui.gameplay_gui')

local vdom_root = vdom.render(vdom.create_element(gameplay_gui, gui_state), root_widget)

function update_gui_state()
    vdom.render(vdom.create_element(gameplay_gui, gui_state), root_widget, vdom_root)
    vdom.flush_updates()
end
