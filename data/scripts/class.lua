return function (base)
    local c = {}
    c.__index = c

    if base then
        setmetatable(c, { __index = base })
        function c:super(...)
            if base.constructor then
                base.constructor(self, ...)
            elseif base.super then
                base.super(self, ...)
            end
        end
    end

    function c.new(...)
        local o = {}
        setmetatable(o, c)
        if c.constructor then
            o:constructor(...)
        elseif c.super then
            o:super(...)
        end
        return o
    end

    return c
end

