local _table = table
-----------------------------------------------------
local function parseargs(s)
    local arg = {}
    string.gsub(s, "([%-%w_]+)=([\"'])(.-)%2", function (w, _, a)
        arg[w] = a
    end)
    return arg
end
function _table.xml(s)
    local stack = {}
    local top = {}
    table.insert(stack, top)
    local ni,c,label,xarg, empty
    local i, j = 1, 1
    while true do
        ni,j,c,label,xarg, empty = string.find(s, "<(%/?)([%w:]+)(.-)(%/?)>", i)
        if not ni then break end
        local text = string.sub(s, i, ni-1)
        if not string.find(text, "^%s*$") then
            table.insert(top, text)
        end
        if empty == "/" then  -- empty element tag
            table.insert(top, {label=label, xarg=parseargs(xarg), empty=1})
        elseif c == "" then   -- start tag
            top = {label=label, xarg=parseargs(xarg)}
            table.insert(stack, top)   -- new level
        else
            -- end tag
            local toclose = table.remove(stack)  -- remove top
            top = stack[#stack]
            if #stack < 1 then
                error("nothing to close with "..label)
            end
            if toclose.label ~= label then
                error("trying to close "..toclose.label.." with "..label)
            end
            table.insert(top, toclose)
        end
        i = j+1
    end
    local text = string.sub(s, i)
    if not string.find(text, "^%s*$") then
        table.insert(stack[#stack], text)
    end
    if #stack > 1 then
        error("unclosed "..stack[#stack].label)
    end
    return stack[1]
end
-----------------------------------------------------
local function dump(t,space,name, cache)
    local temp = {}
    for k,v in pairs(t) do
        local key = tostring(k)
        if cache[v] then
            table.insert(temp,"+" .. key .. " {" .. cache[v].."}")
        elseif type(v) == "table" then
            local new_key = name .. "." .. key
            cache[v] = new_key
            table.insert(temp,"+" .. key .. dump(v,space .. (next(t,k) and "|" or " " ).. string.rep(" ",#key),new_key))
        else
            table.insert(temp,"+" .. key .. " [" .. tostring(v).."]")
        end
    end
    return table.concat(temp,"\n"..space)
end
-- 打印表
function _table.dump(root, space, name)
    return dump(root, space or "", name or "", { [root] = "." })
end
-----------------------------------------------------
return _table
