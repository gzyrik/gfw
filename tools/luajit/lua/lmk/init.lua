local path= require("lmk.path")
local sync= require("lmk.sync")
local os  = require("lmk.os")
local lmk={}
lmk.m={}
lmk.path=path
lmk.sync=sync
lmk.os=os
lmk.vs9_build = path.join(path.HOME, 'vs9_build')
-----------------------------------------------------------
local function vs9_build (vs9)
    if #vs9 ~= 2 then return nil, "vs9 param error" end
    local cmd = {lmk.vs9_build, '/nologo', vs9[1]}
    if vs9[2] then
        table.insert(cmd, '/project')
        table.insert(cmd, vs9[2])
    end
    if os.arg.rebuild then
        table.insert(cmd,'/rebuild')
    else    
        table.insert(cmd,'/build')
    end
    if os.arg.debug then
        table.insert(cmd,'Debug')
    else
        table.insert(cmd,'Release')
    end
    --table.insert(cmd,'>NUL')
    return os.system(table.concat(cmd, ' '))
end
-----------------------------------------------------------
local function xcode_build(sln, proj, config)
    local cmd= 'xcodebuild -project ' .. proj .. ' -target ' .. target .. '-configuration ' .. config
    return os.system(cmd)
end
-----------------------------------------------------------
local function mk_build(cmd)
    local dir, name= path.split(cmd)
    cmd = dir and {'cd ',dir, '&&'} or {name}
    if os.arg.rebuild then table.insert(cmd, '-B') end
    if os.arg.debug then table.insert(cmd, 'NDK_DEBUG=1') end
    for k,v in pairs(os.arg) do
        if string.match(k,'^NDK_%w+$') then
            table.insert(cmd, k..'='..v)
        end
    end
    return os.system(table.concat(cmd, ' '))
end
-----------------------------------------------------------
local function cmd_build(cmd)
    local dir, name= path.split(cmd)
    if dir then cmd = 'cd ' .. dir .. ' && ' .. cmd end
    return os.system(cmd)
end
-----------------------------------------------------------
local function lua_build(cmd)
    os.echo(8, cmd)
    local dir, name= path.split(cmd, '.')
    local cwd = os.cwd()
    os.cd(dir)
    local ret = dofile(name)
    os.cd(cwd)
    if not ret then return nil, 'error: '..cmd end
    return true
end
-----------------------------------------------------
-- lmk 模块
-----------------------------------------------------
local function _build(n, t, b)
    if b[n] then
        return true
    else
        b[n] = true
    end
    local m = lmk.m[n] or _G[n]
    if not m then
        os.echo(2, 'Warning ', n, ' is not exist!')
        return true
    end
    local ret, err
    local deps = m.deps or m.dependencies
    if deps then
        for k, v in pairs(deps) do
            local sm = lmk.m[k] or _G[k]
            if sm then
                ret, err = _build(k, t, b)
                if not ret then return ret, err end
                if v.inc and sm.inc then
                    local src_dir = path.join(sm.home, sm.inc.dir)
                    local dst_dir = path.join(m.home, v.inc)
                    os.echo(6, n, ' sync inc: ', src_dir, ' -> ', dst_dir)
                    for _, sv in pairs(sm.inc) do
                        if type(_) == 'number' then
                            ret, err = sync(path.join(src_dir, sv), path.join(dst_dir, path.split(sv)))
                            if not ret then return ret, err end
                        end
                    end
                end
                if v.lib and v.lib[t] and sm.lib and sm.lib[t] then
                    local src_dir = path.join(sm.home, sm.lib[t].dir)
                    local dst_dir = path.join(m.home, v.lib[t])
                    os.echo(6, n, ' sync lib: ', src_dir, ' -> ', dst_dir)
                    for _, sv in pairs(sm.lib[t]) do
                        if type(_) == 'number' then
                            ret, err = sync(path.join(src_dir, sv), path.join(dst_dir, sv))
                            if not ret then return ret, err end
                        end
                    end
                end
                if v.bin and v.bin[t] and sm.bin and sm.bin[t] then
                    local src_dir = path.join(sm.home, sm.bin[t].dir)
                    local dst_dir = path.join(m.home, v.bin[t])
                    os.echo(6, n, ' sync bin: ', src_dir, ' -> ', dst_dir)
                    for _, sv in pairs(sm.bin[t]) do
                        if type(_) == 'number' then
                            ret, err = sync(path.join(src_dir, sv), path.join(dst_dir, sv))
                            if not ret then return ret, err end
                        end
                    end
                end
            end
        end
    end
    os.echo(4, '========== Building [', n, '] at: ', m.home, ' ==========')
    local cwd = os.cwd()
    os.cd(m.home)
    os.echo(5, 'change dir:', m.home)
    if m.pre_build then m.pre_build(m, t) end
    local c=m[t]
    if c then
        if c.vs9 then
            ret, err = vs9_build(c.vs9)
        elseif c.mk then
            ret, err = mk_build(c.mk)
        elseif c.cmd then
            ret, err = cmd_build(c.cmd)
        elseif c.lua then
            ret, err = lua_build(c.lua)
        else
            ret, err = nil, 'cannot support ' .. t
        end
        if not ret then return ret, err end
    end
    if m.post_build then m.post_build(m, t) end
    os.cd(cwd)
    os.echo(5, 'change dir:', cwd)
    return true
end
function lmk.build(m, t)
    return _build(m, t, {})
end
-----------------------------------------------------
-- 注册模块
function lmk.__call(obj, m)
    m.home = path.split(debug.getinfo(2, 'S').source:sub(2))
    if not m.name then _, m.name = path.split(m.home) end
    if lmk.m[m.name] then
        return nil, os.echo(0, 'error:', 'already exist module ', m.name)
    end
    os.echo(4, 'add [', m.name, '] module at: ', m.home)
    lmk.m[m.name] = m
    return m
end
-----------------------------------------------------
setmetatable(lmk, lmk)
return lmk
