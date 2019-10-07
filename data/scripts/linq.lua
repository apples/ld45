
local linq = {}
local where_iter = {}
local select_iter = {}
local concat_iter = {}
local drop_iter = {}

-- linq base

linq.__index = linq

setmetatable(
    linq,
    {
        __call = function (cls, ...)
            return cls.new(...)
        end
    }
)

function linq.new(seq, len)
    if not seq then
        error("seq must not be null", 2)
    end

    local self = setmetatable({}, linq)
    self.source = seq
    self.index = 0
    self.length = len or #seq
    return self
end

function linq:__call()
    self.index = self.index + 1
    if self.index <= self.length then
        return self.source[self.index]
    end
end

-- methods

function linq:first()
    return self()
end

function linq:tolist()
    local list = {}
    for v in self do
        list[#list + 1] = v
    end
    return list
end

function linq:reduce(accum, func)
    for v in self do
        accum = func(accum, v)
    end
    return accum
end

function linq:where(pred)
    return where_iter(self, pred)
end

function linq:select(mapper)
    return select_iter(self, mapper)
end

function linq:concat(seq)
    return concat_iter(self, seq)
end

function linq:drop(count)
    return drop_iter(self, count)
end

-- where iterator

where_iter.__index = where_iter

setmetatable(
    where_iter,
    {
        __index = linq,
        __call = function (cls, ...)
            return cls.new(...)
        end
    }
)

function where_iter.new(source, pred)
    local self = setmetatable({}, where_iter)
    self.source = source
    self.predicate = pred
    return self
end

function where_iter:__call()
    local x = self.source()
    while x ~= nil do
        if self.predicate(x) then
            return x
        else
            x = self.source()
        end
    end
end

-- select iterator

select_iter.__index = select_iter

setmetatable(
    select_iter,
    {
        __index = linq,
        __call = function (cls, ...)
            return cls.new(...)
        end
    }
)

function select_iter.new(source, mapper)
    local self = setmetatable({}, select_iter)
    self.source = source
    self.mapper = mapper
    self.index = 0
    return self
end

function select_iter:__call()
    self.index = self.index + 1
    local x = self.source()
    if x ~= nil then
        return self.mapper(x, self.index)
    end
end

-- concat iterator

concat_iter.__index = concat_iter

setmetatable(
    concat_iter,
    {
        __index = linq,
        __call = function (cls, ...)
            return cls.new(...)
        end
    }
)

function concat_iter.new(source1, source2)
    local self = setmetatable({}, concat_iter)
    self.sources = {source1, source2}
    self.current = 1
    return self
end

function concat_iter:__call()
    local x = self.sources[self.current]()
    while x == nil and self.current < #self.sources do
        self.current = self.current + 1
        x = self.sources[self.current]()
    end
    if x ~= nil then
        return x
    end
end

-- drop iterator

drop_iter.__index = drop_iter

setmetatable(
    drop_iter,
    {
        __index = linq,
        __call = function (cls, ...)
            return cls.new(...)
        end
    }
)

function drop_iter.new(source, count)
    local self = setmetatable({}, drop_iter)
    self.source = source
    self.count = count
    return self
end

function drop_iter:__call()
    while self.count > 0 do
        self.source()
        self.count = self.count - 1
    end
    return self.source()
end

return linq
