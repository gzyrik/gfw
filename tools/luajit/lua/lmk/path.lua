local lfs = require("lfs")
local os  = require("lmk.os")
local path= {}
path.SEP = package.config:sub(1,1)
-----------------------------------------------------
-- 抽取路径名,文件名
function path.split (p, default)
    local dir, name
    if p then
        if path.SEP ~= '/' then p = p:gsub(path.SEP,'/') end
        dir = p:match('^([^=]+)/')
        if dir then
            dir, name = p:match('^(.+)/([^/]*)$')
        else
            name = p
        end
    end
    dir = dir or default
    if dir and path.SEP ~= '/' then dir = dir:gsub('/',path.SEP) end
    return dir, name
end
path.HOME= path.split(debug.getinfo(1).source:sub(2), '.')
-----------------------------------------------------
-- 构造路径
function path.join(...)
    local arg={...}
    local ret = {}
    for _,v in ipairs(arg) do
        if v then
            if path.SEP ~= '/' then v = v:gsub(path.SEP,'/') end
            --'.'=46,'/'=47,':'=58,'a'=97,'z'=122
            local a,b,c=v:byte(1,3)

            --绝对路径 '/'or '[a-z]:', or '/[a-z]:'
            if a== 47 or b == 58 or c == 58 then 
                if #ret > 0 then return v end
            end

            --忽略 '.'
            if v == '.' then
                v = nil
            else
                --跳过起始 './'
                if a == 46 and b == 47 then v = v:sub(3,-1) end
                --删除末尾 '/'
                if v:byte(-1) == 47 then v = v:sub(1, -2) end
            end

            if v then
                if path.SEP ~= '/' then v = v:gsub('/',path.SEP) end
                table.insert(ret, v)
            end
        end
    end
    if #ret == 0 then
        return '.'
    else
        return table.concat(ret, path.SEP)
    end
end

-----------------------------------------------------
-- 判断是否目录
function path.isdir(p)
    if path.SEP ~= '/' then p = p:gsub(path.SEP,'/') end
    if p:match('/$') then p = p:sub(1,-2) end
    return lfs.attributes(p,'mode') == 'directory'
end
-----------------------------------------------------
-- 判断是否目录
function path.isequal(p1, p2)
    if path.SEP ~= '/' then
        p1 = p1:gsub(path.SEP,'/')
        p2 = p2:gsub(path.SEP,'/')
    end
    print('...', p1, p2)
    return p1 == p2;
end
-----------------------------------------------------
local function _mkdir(p)
    if path.SEP ~= '/' and p:find '^%a:/*$' then -- Windows root drive case
        return true
    end
    if not path.isdir(p) then
        local subp = p:match '^(.+)/[^/]+$'
        if subp and not _mkdir(subp) then
            return nil,'cannot create '..subp
        end
        os.echo(6, 'mkdir:', p)
        return lfs.mkdir(p)
    else
        return true
    end
end
-- 创建目录
function path.mkdir (p)
    if path.SEP ~= '/' then p = p:gsub(path.SEP,'/') end
    return _mkdir(p)
end
-----------------------------------------------------
function _walk(dir, mode, func, level)
    for file in lfs.dir(dir) do
        if file ~= "." and file ~= ".." then
            local p = path.join(dir, file)
            if lfs.attributes(p, 'mode') == 'directory' then
                if mode.depth  and level < mode.depth and
                    (not mode.dir or file:math(mode.dir)) then
                    ret, err = _walk(p, mode, func, level+1)
                    if not ret then return ret, err end
                end
            elseif not mode.file or file:match(mode.file) then
                ret, err = func (path.join(dir, file))
                if not ret then return ret, err end
            end
        end
    end
    return true
end
-- 遍历目录
function path.walk(dir, mode, func)
    return _walk(dir, mode, func, 0)
end
-----------------------------------------------------
-- 无参数时,返回当前脚本所在目录
function path.__call(obj, ...)
    if select('#', ...) > 0 then
        return path.join(...)
    else
        local f= path.split(debug.getinfo(2, 'S').source:sub(2))
        return path.join(os.cwd(),f)
    end
end
-----------------------------------------------------
setmetatable(path, path)
return path
