local linq = require('linq')
local class = require('class')

local function assign(dest, source)
    for k,v in pairs(source) do
        dest[k] = v
    end
    return dest
end

local function is_event_name(str)
    return string.sub(str, 1, 3) == 'on_'
end

-- Updates the widget events and attributes based on the props
local function update_widget_properties(widget, prev_props, next_props)
    for k,v in pairs(prev_props) do
        if next_props[k] == nil then
            if is_event_name(k) then
                widget[k] = nil
            else
                widget:set_attribute(k, nil)
            end
        end
    end
    for k,v in pairs(next_props) do
        if prev_props[k] ~= v then
            if is_event_name(k) then
                widget[k] = v
            else
                widget:set_attribute(k, type(v) == 'string' and v or tostring(v))
            end
        end
    end
end

-- Determines if the given object is either a function or a class derived from component_base
local function is_component_type(cls)
    return type(cls) == 'function'
end

-- Determines if the argument is a cdom element created by create_element
local function is_vdom_element(element)
    return type(element) == 'table' and
        (type(element.type) == 'string' or is_component_type(element.type)) and
        type(element.props) == 'table' and
        element.children ~= nil
end

-- Constructs a new component instance
local function create_component_instance(element, instance)
    assert(is_vdom_element(element))
    assert(type(instance) == 'table')

    local element_type = element.type
    local props = element.props
    local children = element.children

    local component_instance = element_type.new(props, children)
    component_instance.internal_instance = instance

    return component_instance
end

-- Determines if the argument is an instance
local function is_instance(instance)
    return type(instance) == 'table' and
        instance.widget ~= nil and
        is_vdom_element(instance.element) and (
            type(instance.child_instances) == 'table' or -- widget instance
            is_vdom_element(instance.child_element) and is_instance(instance.child_instance) -- component instance
        )
end

-- Determines if the argument is a widget (non-component) instance
local function is_widget_instance(instance)
    return is_instance(instance) and
        type(instance.child_instances) == 'table'
end

-- [private] The currently-rendering instances (used by hooks)
local current_instance = {}
local current_instance_progress = {}

-- Sets the currently-rendering instance
local function set_current_instance(instance)
    assert(#current_instance < 1024) -- Infinite recursion safeguard
    assert(type(instance) == 'table')

    table.insert(current_instance, instance)
    table.insert(current_instance_progress, {
        hook_index = 1,
        effect_index = 1
    })
end

-- Clears the currently-rendering instance
local function clear_current_instance()
    assert(#current_instance > 0)
    assert(#current_instance_progress > 0)

    table.remove(current_instance)
    table.remove(current_instance_progress)
end

-- Gets the currently-rendering instance and its progress
local function get_current_instance()
    assert(#current_instance > 0)
    assert(#current_instance_progress > 0)

    return current_instance[#current_instance], current_instance_progress[#current_instance_progress]
end

-- Gets or creates the next hook
local function get_next_hook()
    local instance, progress = get_current_instance()

    if instance.hooks == nil then
        instance.hooks = {}
    end

    local hook = instance.hooks[progress.hook_index]

    if hook == nil then
        hook = {}
        instance.hooks[progress.hook_index] = hook
    end

    progress.hook_index = progress.hook_index + 1

    return hook
end

-- Gets or creates the effect at the current index
local function get_next_effect()
    local instance, progress = get_current_instance()

    if instance.effects == nil then
        instance.effects = {}
    end

    local effect = instance.effects[progress.effect_index]

    if effect == nil then
        effect = {}
        instance.effects[progress.effect_index] = effect
    end

    progress.effect_index = progress.effect_index + 1

    return effect
end

local function get_context_provider(instance)
    return instance.context and instance or instance.context_provider
end

-- Instantiates a new instance
local function instantiate(element, parent_widget, context_provider)
    assert(is_vdom_element(element))

    local element_type = element.type
    local props = element.props
    local children = element.children

    if type(element_type) == 'string' then
        -- Instantiate widget element

        if element_type == '_TEXT_ELEMENT_' then
            error('_TEXT_ELEMENT_ not supported')
        end

        local widget = parent_widget:create_widget(element_type)

        update_widget_properties(widget, {}, props)

        local child_instances = linq(children)
        :select(function (c) return instantiate(c, widget, context_provider) end)
            :tolist()

        local child_widgets = linq(child_instances)
            :select(function (child_instance) return child_instance.widget end)
            :tolist()

        for _,child_widget in ipairs(child_widgets) do
            widget:add_child(child_widget)
        end

        return {
            widget = widget,
            element = element,
            child_instances = child_instances,
            context_provider = context_provider
        }
    else
        -- Instantiate component element

        local instance = {}

        instance.context_provider = context_provider

        if not context_provider then
            print('Warning: no context')
        end

        set_current_instance(instance)
        local child_element = element.type(element.props)
        clear_current_instance()

        local child_instance = instantiate(child_element, parent_widget, get_context_provider(instance))

        instance.widget = child_instance.widget
        instance.element = element
        instance.child_element = child_element
        instance.child_instance = child_instance

        return instance
    end
end

-- Updates a component instance with new props
local function update_component(instance, props)
    assert(is_instance(instance))
    assert(type(props) == 'table')

    set_current_instance(instance)
    local child = instance.element.type(props)
    clear_current_instance()

    return child
end

local reconcile -- forward

-- Reconciles children of a widget instance
local function reconcile_children(instance, element)
    assert(is_widget_instance(instance))
    assert(is_vdom_element(element))

    local widget = instance.widget
    local child_instances = instance.child_instances
    local child_context_provider = get_context_provider(instance)
    local next_child_elements = element.children
    local new_child_instances = {}
    local count = math.max(#child_instances, #next_child_elements)

    for i = 1, count do
        local child_instance = child_instances[i]
        local child_element = next_child_elements[i]
        local new_child_instance = reconcile(widget, child_instance, child_element, child_context_provider)

        if new_child_instance then
            new_child_instances[#new_child_instances + 1] = new_child_instance
        end
    end

    return new_child_instances
end

-- Cleans up references to prevent cycles (this might be better resolved with weak references)
local function cleanup(instance)
    assert(is_instance(instance))

    if instance.effects then
        for _,effect in ipairs(instance.effects) do
            assert(type(effect.on_unmount) == 'function')
            effect.on_unmount()
        end
    end

    if instance.child_instance then
        cleanup(instance.child_instance)
    end

    if instance.child_instances then
        for _,v in ipairs(instance.child_instances) do
            cleanup(v)
        end
    end

    instance.widget = nil
end

reconcile = function (parent_widget, instance, element, context_provider)
    assert(parent_widget ~= nil)
    assert(instance == nil or is_instance(instance))
    assert(element == nil or is_vdom_element(element))

    if instance == nil then
        -- Create instance

        local new_instance = instantiate(element, parent_widget, context_provider)

        parent_widget:add_child(new_instance.widget)

        return new_instance
    elseif element == nil then
        -- Remove instance

        parent_widget:remove_child(instance.widget)
        cleanup(instance)

        return nil
    elseif instance.element.type ~= element.type then
        -- Replace instance

        local new_instance = instantiate(element, parent_widget, get_context_provider(instance))

        parent_widget:replace_child(instance.widget, new_instance.widget)
        cleanup(instance)

        return new_instance
    elseif type(element.type) == 'string' then
        -- Update instance

        update_widget_properties(instance.widget, instance.element.props, element.props)

        instance.child_instances = reconcile_children(instance, element)
        instance.element = element

        return instance
    else
        -- Update composite instance

        local child_element = update_component(instance, element.props)

        if child_element ~= instance.child_element then
            local old_child_instance = instance.child_instance
            local child_instance = reconcile(
                parent_widget,
                old_child_instance,
                child_element,
                get_context_provider(instance))

            instance.widget = child_instance.widget
            instance.child_element = child_element
            instance.child_instance = child_instance
            instance.element = element
        end

        return instance
    end
end

local function shallow_equals(a, b)
    local n = 0
    for k,v in pairs(a) do
        if b[k] ~= v then return false end
        n = n + 1
    end
    for k,v in pairs(b) do
        n = n - 1
    end
    return n == 0
end

-- Instances queued for update
local queued_updates = {}

local function queue_update(instance)
    assert(is_instance(instance))

    table.insert(queued_updates, instance)
end

local function flush_updates()
    while #queued_updates > 0 do
        local instance = table.remove(queued_updates)

        if instance.widget then
            local parent_widget = instance.widget:get_parent()
            local element = instance.element

            reconcile(parent_widget, instance, element, get_context_provider(instance))
        end
    end
end

local function create_element(element_type, config, ...)
    assert(type(element_type) == 'string' or is_component_type(element_type))
    assert(type(config) == 'nil' or type(config) == 'table')

    local props = assign({}, config or {})

    local children = {}

    local function add_child(child)
        if is_vdom_element(child) then
            children[#children + 1] = child
        elseif child then
            children[#children + 1] = create_element('_TEXT_ELEMENT_', { node_value = tostring(child) })
        end
    end

    local count = select('#', ...)

    for i=1,count do
        local child = select(i, ...)

        if type(child) == 'table' and not is_vdom_element(child) then
            for _,v in ipairs(child) do
                add_child(v)
            end
        else
            add_child(child)
        end
    end

    return {
        type = element_type,
        props = props,
        children = children,
    }
end

local function render(element, container, instance)
    assert(is_vdom_element(element))
    assert(container ~= nil)
    assert(instance == nil or is_instance(instance))

    return reconcile(container, instance, element, instance and get_context_provider(instance))
end

local function useContextProvider(init)
    local instance = get_current_instance()

    if instance.context == nil then
        instance.context = {
            state = init(),
            setState = function (newState)
                instance.context.state = newState
                for _,subscriber in ipairs(instance.context.subscribers) do
                    queue_update(subscriber)
                end
            end,
            subscribers = {}
        }
    end

    return instance.context.setState, instance.context.state
end

local function useContext()
    local instance = get_current_instance()

    assert(instance.context_provider)

    local effect = get_next_effect()

    if effect.on_unmount == nil then
        local context = instance.context_provider.context
        context.subscribers[#context.subscribers + 1] = instance

        effect.context = context
        effect.on_unmount = function ()
            for i,subscriber in ipairs(context.subscribers) do
                if subscriber == instance then
                    table.remove(context.subscribers, i)
                    break
                end
            end
        end
    end

    return effect.context.state
end

local function useState(init)
    local instance = get_current_instance()
    local hook = get_next_hook()

    if hook.setState == nil then
        if type(init) == 'function' then
            hook.state = init()
        else
            hook.state = init
        end
        hook.setState = function (newState)
            hook.state = newState
            queue_update(instance)
        end
    end

    return hook.state, hook.setState
end

local function useMemo(func, deps)
    local hook = get_next_hook()

    local function needs_recalc()
        return not shallow_equals(deps, hook.deps)
    end
    
    if hook.deps == nil or needs_recalc() then
        hook.value = func()
        hook.deps = deps
    end

    return hook.value
end

local function useCallback(func, deps)
    return useMemo(function () return func end, deps)
end

return {
    create_element = create_element,
    render = render,
    flush_updates = flush_updates,
    useContextProvider = useContextProvider,
    useContext = useContext,
    useState = useState,
    useMemo = useMemo,
    useCallback = useCallback
}
