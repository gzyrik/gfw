local lfs = require("lfs")
local path= require("lmk.path")
local os  = require("lmk.os")
local sync= {}
-----------------------------------------------------
-- 同步文件 src -> dst
-- mode:
--      'nocreate':不创建新文件
--      'force':强制覆盖
-- return:
--      nil 出错
--      1   更新文件
--      0   无操作

function sync.file(src, dst, mode)
    mode = mode or {}
    os.echo(9, 'sync[F]:', src, '->', dst)

    local src_attri = lfs.attributes(src)
    local dst_attri = lfs.attributes(dst)
    if not dst_attri then
        if mode.nocreate then return 0 end
        path.mkdir(path.split(dst))
    elseif src_attri.modification ~= dst_attri.modification  then
        if not mode.force and src_attri.modification < dst_attri.modification then
            return 0 
        end
    end
    os.echo(6, 'copy:', src, '->', dst)

    local dst_file = io.open(dst,'w+b')
    if not dst_file then return nil, 'cannot open '..dst end

    local file = io.open(src, 'rb')
    if not file then return nil, 'cannot open '..src end

    dst_file:write(file:read('*a'))
    file:close()
    dst_file:close()

    return 1 
end

-----------------------------------------------------
-- 同步头文件 head -> dst
-- mode:
--      'nocreate':不创建新文件
--      'force':强制覆盖
--      'head_quick':若该头文件无需更新,则也无需解析
-- return:
--      nil 出错
--
function sync.head(head, dst, mode)
    mode = mode or {}
    os.echo(9, 'sync[H]:', head, '->', dst)

    local base, name =path.split(head, '.')
    local ret, err = path.mkdir(dst)
    if not ret then return ret, err end

    ret, err = sync.file(head, path.join(dst, name), mode)
    if not ret then return ret, err end
    if ret == 0 and mode.head_quick then return true end

    --读取head, 并删除注释
    local file=io.open(head)
    local str =file:read('*a')
    file:close()
    str=str:gsub('/%*.-%*/','')
    str=str:gsub('//.-\n','')

    --遍历#include
    for i in str:gmatch('#include%s*"(.-)"') do
        local dir, name = path.split(i)
        if dir then
            ret, err = path.mkdir(path.join(dst, dir))
            if not ret then return ret, err end
        else
            dir = '.'
        end
        ret, err = sync.head(path.join(base, dir, name), path.join(dst, dir), mode)
        if not ret then return ret, err end
    end
    return true
end
-----------------------------------------------------
-- 同步目录
-- mode :
--      'depth': 递归深度
--      'head': 处理头文件
--      'head_quick':若该头文件无需更新,则也无需解析
--      'nocreate':不创建新文件
--      'file':文件名正则匹配
--      'dir':目录名正则匹配
-- return:
--      nil 出错
local function sync_dir(src, dst, mode, level)
    mode = mode or {}
    os.echo(9, 'sync[D]:', src, '->', dst)

    local ret, err = path.mkdir(dst)
    if not ret then return ret, err end

    for file in lfs.dir(src) do
        if file ~= "." and file ~= ".." then
            local p = path.join(src, file)
            if lfs.attributes(p,'mode') == 'directory' then
                if mode.depth and level < mode.depth and
                    (not mode.dir or file:math(mode.dir)) then
                    ret, err = sync_dir(p, path.join(dst,file), mode, level+1)
                    if not ret then return ret, err end
                end
            elseif not mode.file or file:match(mode.file) then
                if mode.head and file:match('^.-%.[Hh]$') then
                    ret, err = sync.head (p, dst, mode)
                else
                    ret, err = sync.file (p, path.join(dst, file), mode)
                end
                if not ret then return ret, err end
            end
        end
    end
    return true
end
function sync.dir(src, dst, mode)
    return sync_dir(src, dst, mode, 0)
end
-----------------------------------------------------
-- 自动同步,处理头文件 /** 表示递归, 末尾带正则模式
--
function sync.__call(obj, src, dst)
    local r
    local mode={head=true}

    if path.SEP ~= '/' then src = src:gsub('\\','/') end
    os.echo(9, 'sync :', src, '->', dst)

    src,r = src:gsub('/%*%*', '')
    if r > 0 then mode.depth = 1024 end

    local attr = lfs.attributes (src)
    if not attr then 
        local sd,sp = src:match('^(.-)/([^/]+)$')
        if path.isdir(sd) then
            print(sd,sp)
            if sp and sp:find("[+-%*]") then
                mode.file = sp
                return sync.dir(sd, dst, mode)
            else
                return nil, 'no pattern by '..src
            end
        else
            return nil, 'cannot open '..sd
        end
    elseif attr.mode == "directory" then
        return sync.dir(src, dst, mode)
    elseif src:match('^.-%.[Hh]$') then
        return sync.head(src, dst, mode)
    else
        return sync.file(src, dst, mode)
    end
end
-----------------------------------------------------
setmetatable(sync, sync)
return sync

