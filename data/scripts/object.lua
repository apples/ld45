local object = {}

function object.pick(src, ...)
    local rv = {}
    for _,k in ipairs({...}) do
        rv[k] = src[k]
    end
    return rv
end

function object.assign(dest, src)
    for k,v in pairs(src) do
        dest[k] = v
    end
    return dest
end

return object
