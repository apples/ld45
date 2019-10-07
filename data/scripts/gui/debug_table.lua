local vdom = require('vdom')

local function textrow(props)
    assert(props.index)
    assert(props.text)
    assert(props.top)

    local text = props.text

    return vdom.useMemo(
        function ()
            return vdom.create_element(
                'label',
                {
                    halign = props.align,
                    valign = 'top',
                    height = 12,
                    color = '#fff',
                    text = text,
                    top = props.top + 12 * (props.index - 1),
                    left = props.left,
                    right = props.right,
                }
            )
        end,
        { text, props.align, props.top, props.left, props.right, props.index }
    )
end

return function(props)
    assert(props.strings)
    assert(props.left or props.right)
    assert(props.top)

    local context = vdom.useContext()

    local myStrings = context[props.strings]

            local rv = {}
            for i=1,#myStrings do
                table.insert(rv, vdom.create_element(textrow, {
                    index = i,
                    text = myStrings[i],
                    align = props.align,
                    top = props.top,
                    left = props.left,
                    right = props.right
                }))
            end

        return vdom.create_element(
            'panel',
            {
                halign='left',
                valign='top',
            },
            rv
        )
end
