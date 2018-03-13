local lfs=require('lfs')
local hk={}
local opt={}
local SEP  = package.config:sub(1,1)
local unix = (SEP == '/')
os.cd = lfs.chdir
os.pwd = lfs.currentdir
function os.stat(path)
    if not path then return nil end
    --del last '/'
    path = path:gsub('%c','')
    if path:byte(-1) == 47 then path=path:sub(1,-2) end 
    local attri = lfs.attributes(path)
    if not attri then return nil  end
    return {
        time = attri.modification,
        dir  = (attri.mode == 'directory'),
        size = attri.size,
        name = path:gsub('^(.+)[/\\]',''),
        path = path,
    }
end
local function mk_path(p, ...)
    local arg={...}
    local ret
    if p == '.' then
        ret = nil
    else
        ret = p
    end
    for _,v in pairs(arg) do
        --'.'=46,'/'=47,':'=58,'a'=97,'z'=122
        local a,b,c=v:byte(1,3)

        -- prefix '/', or '[a-z]:', or '/[a-z]: is absolute path
        if a== 47 or b == 58 or c == 58 then 
            return v
        end

        --skip './'
        if a == 46 and b == 47 then
            v = v:sub(3,-1)
        end

        if not v then
        elseif not ret then
            ret = v
        elseif ret:byte(-1) ~= 47 then
            ret = ret .. '/' .. v
        else
            ret = ret .. v
        end
    end
    return ret or '.'
end
function os.dir(path)
    local d,o = lfs.dir(path)
    return function ()
        local f = d(o)
        return f and os.stat(mk_path(path, f))
    end
end
-----------------------------------------------------------
local verbose_level={
    debug = 4,
    info  = 3,
    title = 2,
    warn  = 1,
    error = 0
}
local level_color={
    debug = '\027[1;40;32m',
    info  = '\027[1;40;36m',
    title = '\027[1;40;34m',
    warn  = '\027[1;40;33m',
    error = '\027[1;40;31m'
}
local function print(str, level)
    level = level or 'debug'
    if verbose_level[hk.arg.HK_VERBOSE] < verbose_level[level] then
        return
    end
    if unix and level then io.write(level_color[level]) end
    io.write(str);
    if unix then io.write('\027[0m') end
    io.write('\n')
end
-----------------------------------------------------------
local function execute(cmd, level)
    print(cmd, level or 'debug');
    if not hk.arg.HK_DRYRUN then
        if not os.execute(cmd) then
            print("***************************************", 'error') 
            print(" ERROR: "..cmd,                           'error')
            print(" Press 'c' to continue, other to abort!", 'error')
            print("***************************************", 'error') 
            if io.read():lower() ~= 'c' then os.exit(-1) end
        end
    end
end
-----------------------------------------------------------
local function copy_file (src, dst)
    local cmd
    if unix then
        cmd = 'cp -p ' .. src .. ' ' .. dst
    else
        cmd = 'copy ' .. string.gsub(src,'/','\\') .. ' ' .. string.gsub(dst,'/','\\') .. '>NUL'
    end
    execute(cmd)
end
local function remove_file (src)
    local cmd
    if unix then
        cmd = 'rm ' .. src .. ' 2>/dev/null || true '
    else
        cmd = 'del ' .. string.gsub(src,'/','\\') .. '>NUL'
    end
    execute(cmd)
end
local function mk_dir (dir)
    local cmd
    if unix then
        cmd = 'mkdir -p ' .. dir
    else
        local d = string.gsub(dir,'/','\\')
        cmd = 'if not exist ' .. d .. ' (mkdir ' .. d .. ')'
    end
    execute(cmd);
end

local function dir_name(path)
    path = path:gsub('\\','/')
    return path:match('^([^=]+)/[^/]*$') or '.'
end

--返回当前脚本所在目录
local function local_dir()
    local f=debug.getinfo(2, 'S').source:sub(2)
    return mk_path( os.pwd(), dir_name(f))
end
--是否为入口脚本
local function main_entry()
    local f=debug.getinfo(2, 'S').source:sub(2)
    if not arg[0]:find('\\') then
        f=f:gsub('\\','/')
    end
    return arg[0] == f
end

--hk.lua文件所在目录
local home_dir = debug.getinfo(1).source:sub(2)
home_dir = dir_name(home_dir)

local function find_file(f, dir)
    for _, d in pairs(dir) do
        local s = os.stat(d .. '/' .. f)
        if s then return s end
    end
end
-----------------------------------------------------------
local function sln_build (sln, proj, config)
    local cmd = mk_path(home_dir, 'vc_build.bat')
    if hk.arg.HK_REBUILD then
        cmd = cmd:gsub('/','\\') .. ' "' .. sln .. '" /Rebuild "' .. config .. '" /project "' .. proj .. '"'
    else    
        cmd = cmd:gsub('/','\\') .. ' "' .. sln .. '" /build "' .. config .. '" /project "' .. proj .. '"'
    end
    execute(cmd, 'title')
end
local function xcode_build(sln, proj, config)
    local cmd= 'xcodebuild -project ' .. proj .. ' -target ' .. target .. '-configuration ' .. config
    execute(cmd, 'title')
end
local function cmd_build(cmd)
    local dir = dir_name(cmd)
    cmd = cmd:match('.-([^/]+)$')
    if dir == '' or dir == nil then dir = '.' end
    cmd = 'cd ' .. dir .. ' && ' .. cmd
    if hk.arg.HK_REBUILD then
        cmd = cmd .. ' -B'
    end
    execute(cmd, 'title')
end
local function lua_build(cmd)
    local dir = dir_name(cmd)
    cmd = cmd:match('.-([^/]+)$')
    if dir == '' or dir == nil then dir = '.' end
    cmd = 'cd ' .. dir .. ' && lua ' .. cmd .. ' ' .. table.concat(opt, ' ')

    if hk.arg.HK_REBUILD then
        cmd = cmd .. ' -B'
    end
    print(cmd, 'title')
    if not os.execute(cmd) then
        print("***************************************", 'error') 
        print(" ERROR: "..cmd,                           'error')
        print(" Press 'c' to continue, other to abort!", 'error')
        print("***************************************", 'error') 
        if io.read():lower() ~= 'c' then os.exit(-1) end
    end
end
-----------------------------------------------------------
local function _sync_file(s, dst)
    assert(not s.dir, s.path .. 'is not a file')
    local d_path = mk_path(dst, s.name)
    local d = os.stat(d_path)
    if not d then
        mk_dir(dst)
        d = os.stat(d_path)
    end
    -- 同步当前文件
    if d == nil or s.time ~= d.time or s.size ~= d.size then
        copy_file(s.path, d_path)
    end 
end
--导出所有引用的文件
--s 文件属性表
--dst 目录名
--dirs 查找目录表
local function _sync_cheader(s, dst, dirs)
    _sync_file(s, dst)
    -- 分析文件,同步引用到的文件
    local file=io.open(s.path)
    local str=file:read('*a')
    file:close()
    --去掉注释
    str=string.gsub(str,'/%*.-%*/','')
    str=string.gsub(str,'//.-\n','')
    --
    local s_dir  = dir_name(s.path)
    for i in string.gmatch(str, '#include%s-"(.-)"') do
        local f_path=mk_path(s_dir, i)
        local d_path=mk_path(dst, dir_name(i))
        --print ('include "' .. f_path .. '" -> ' .. d_path)
        local f = os.stat(f_path)
        if f==nil then f = find_file(i, dirs) end
        if f then
            _sync_cheader(f, d_path, dirs)
        else
            --print('cannot find file: ' .. i)
        end
    end
    --
    for i in string.gmatch(str, '#include%s-<(.-)>') do
        --print ('include <' .. i .. '>')
        local f = find_file(i, dirs)
        local d_path=mk_path(dst, dir_name(i))
        if f then
            _sync_cheader(f, d_path, dirs)
        else
            --print('cannott find file: ' .. i)
        end
    end
end

local function sync_cheader(src, dst, dirs)
    print('sync head: ' .. src .. '\n\t=> ' .. dst, 'info')
    if not dirs then dirs={} end
    _sync_cheader(os.stat(src), dst, dirs)
end

local function sync_file(src, dst)
    print('sync file: ' .. src .. '\n\t=> ' .. dst, 'info')
    _sync_file(os.stat(src), dst)
end

--按模式递归更新目录src_dir->dst_dir
--src 目录属性表
--dst_dir 目录名
--pattern 过滤模式
--r ''只更新该目录,否则递归更新
local function sync_dir(src, dst_dir, pattern, r)
    assert(src.dir, src.path .. ' isnot a dir')
    print('sync  dir: ' .. src.path .. '/' .. r .. pattern .. '\n\t=> '.. dst_dir, 'info')
    --
    local dst = os.stat(dst_dir)
    if not dst then
        mk_dir(dst_dir)
        dst = os.stat(dst_dir)
    end
    assert(dst and dst.dir, dst_dir .. ' isnot a dir')
    --
    for s in os.dir(src.path) do
        local d_path = mk_path(dst.path, s.name)
        if s.dir then
            --递归同步目录
            if r~='' and s.name ~= '.' and s.name ~= '..' then
                sync_dir(s, d_path, pattern, r)
            end
        elseif string.match(s.name, pattern) == s.name then
            --过滤后,更新文件
            local d = os.stat(d_path)
            if d == nil or s.time ~= d.time or s.size ~= d.size then 
                copy_file(s.path, d_path)
            end 
        end
    end
end
-----------------------------------------------------------
local function sync(src, dst)
    local r
    src,r = src:gsub('/%*%*', '/')
    if r > 0 then
        r = '**'
    else
        r = ''
    end
    --去掉末尾'/'
    if src:sub(-1) == '/' then src = src:sub(1,-2) end
    local s = os.stat(src)
    if s then
        if s.dir then
            return sync_dir(s, dst, '.+', r)
        elseif s.name:sub(-2,-1)=='.h' then
            return sync_cheader(src, dst)
        else
            return sync_file(src,dst)
        end
    else
        local sd = string.match(src, '(.-)/[^/]+$')
        s = os.stat(sd)
        if s and s.dir then
            local pat = string.match(src, '.-/([^/]+)$')
            return sync_dir(s, dst, pat, r)
        else
            print("***************************************",   'warn')
            print('ErrorSync: ' .. src .. '\n\t=> ' .. dst, 'warn')
            print("***************************************",   'warn')
        end
    end
end

--按模式递归更新目录src_dir->dst_dir
--src 目录属性表
--dst_dir 目录名
--pattern 过滤模式
--r ''只更新该目录,否则递归更新
local function sync_dirx(src, dst_dir, pattern, r)
    assert(src.dir, src.path .. 'isnot a dir')
    print('sync  dir: ' .. src.path .. '/' .. r .. pattern .. '\n\t=> '.. dst_dir, 'info')

    --
    local dst = os.stat(dst_dir)
    if dst == nil then
        return
    end
    assert(src and dst and src.dir and dst.dir, 'must sync dir')
    --
    for s in os.dir(src.path) do
        local d_path = mk_path(dst.path, s.name)
        if s.dir then
            --递归同步目录
            if r~='' and s.name ~= '.' and s.name ~= '..' then
                sync_dirx(s, d_path, pattern, r)
            end
        elseif string.match(s.name, pattern) == s.name then
            --过滤后,更新文件
            local d = os.stat(d_path)
            if d ~= nil and (s.time ~= d.time or s.size ~= d.size) then 
                copy_file(s.path, d_path)
            end 
        end
    end
end
-----------------------------------------------------------
local function syncx(src, dst)
    local r
    src,r = src:gsub('/%*%*', '/')
    if r > 0 then
        r = '**'
    else
        r = ''
    end
    --去掉末尾'/'
    if src:sub(-1) == '/' then src = src:sub(1,-2) end
    local s = os.stat(src)
    if s then
        if s.dir then
            return sync_dirx(s, dst, '.+', r)
        elseif s.name:sub(-2,-1)=='.h' then
            return sync_cheader(src, dst)
        else
            return sync_file(src,dst)
        end
    else
        local sd = string.match(src, '(.-)/[^/]+$')
        s = os.stat(sd)
        if s and s.dir then
            local pat = string.match(src, '.-/([^/]+)$')
            return sync_dir(s, dst, pat, r)
        else
            print("***************************************",   'warn')
            print('Error Sync path: ' .. src .. ' -> ' .. dst, 'warn')
            print("***************************************",   'warn')
        end
    end
end
-----------------------------------------------------------
local function _for_file(p, pattern)
    for s in os.dir(p.path) do
        if s.dir then
            if s.name ~= '.' and s.name ~= '..' then
                _for_file(s, pattern)
            end
        else
            for k, v in pairs(pattern) do
                if string.match(s.name, k) then
                    v(s.path)
                end
            end 
        end
    end
end
local function for_file(path, pattern)
    s = os.stat(path)
    if s then
        _for_file (s, pattern)
    end
end
-----------------------------------------------------------
-- 递归构建项目
-- n 项目
-- t 平台名称
-- b 用于终止递归的状态表, b[n]表示n项目是否已经构建
local function _build(n, t, b)
    if b[n] then
        return
    else
        b[n] = true
    end
    local m = _G[n]
    if not m then
        print("***************************************", 'warn') 
        print('Warning '.. n .. ' is not exist!!!',      'warn')
        print("***************************************", 'warn') 
        return
    end
    --遍历 `依赖'
    if m.dependencies then
        for k, v in pairs(m.dependencies) do
            local sm = _G[k]
            if sm then
                _build(k, t, b) -- 递归构造 `依赖'
                local work_dir = mk_path(m.local_dir, m.work_dir)
                print('****************************************************************', 'info')
                print('   ' .. n .. ' sync dep: ' .. sm.local_dir .. '->' .. work_dir,    'info')
                print('****************************************************************', 'info')
                --同步头文件
                if v.cheaders_dir then
                    for _, sv in pairs(sm.cheaders) do
                        sync(mk_path(sm.local_dir, sm.cheaders_dir, sv), mk_path(work_dir, v.cheaders_dir, dir_name(sv)))
                    end
                end
                --同步库文件
                if v.clibs_dir then
                    for _, sv in pairs(sm.clibs) do
                        sync(mk_path(sm.local_dir, sm.clibs_dir, sv), mk_path(work_dir, v.clibs_dir, dir_name(sv)))
                    end
                end
                --同步二进制文件
                if v.bins_dir then
                    for _, sv in pairs(sm.bins) do
                        sync(mk_path(sm.local_dir, sm.bins_dir, sv), mk_path(work_dir, v.bins_dir, dir_name(sv)))
                    end
                end
            end
        end
    end
    -- 构建当前项目
    print('****************************************************************', 'title')
    print('   ' .. n .. ' building at: ' .. m.local_dir,                      'title')
    print('****************************************************************', 'title')
    local cwd = os.pwd()
    os.cd(m.local_dir)
    if m.pre_build then m.pre_build(m, t) end
    local c=m[t]
    if c then
        if c.sln then
            sln_build(c.sln, c.proj, c.config)
        elseif c.cmd then
            cmd_build(c.cmd)
        elseif c.lua then
            lua_build(c.lua)
        end
    end
    if m.post_build then m.post_build(m, t) end
    os.cd(cwd)
end

-- 构建项目
-- m 项目
-- t 平台名,可行值有: 'ndk', 'vc', 'ios', ...
local function build(m, t)
    if not t then
        if unix then
            t = 'ndk'
        else
            t = 'vc'
        end
    end
    local b = {}
    _build(m, t, b)
    if not unix then os.execute('pause') end
end
-----------------------------------------------------------
hk.unix         = unix
hk.execute      = execute
hk.mk_path      = mk_path
hk.mk_dir       = mk_dir
hk.dir_name     = dir_name
hk.home_dir     = home_dir
hk.local_dir    = local_dir
hk.main_entry   = main_entry
hk.sln_build    = sln_build
hk.sync_cheader = sync_cheader
hk.sync_file    = sync_file
hk.sync_dir     = sync_dir
hk.sync         = sync
hk.syncx        = syncx
hk.build        = build
hk.print        = print
hk.for_file     = for_file
hk.cp           = copy_file
hk.rm           = remove_file
hk.arg          = {}
hk.target       = nil
hk.config       = nil
-----------------------------------------------------------
--定义加载方式
package.path        = '?/haikui.lua;'..package.path
-----------------------------------------------------------
-- process <key>=<value> in command paramenters, except preffix NDK_
-- default words is target, config, HK_REBUILD
-- hk paramenters:
-- HK_DRYRUN
-- HK_REBUILD
-- HK_VERBOSE
for _,a in ipairs(arg) do
    local k, v = string.match(a, '^(.-)=(.+)$')
    if not k then
        if a:byte(1) ~= 45 then -- '-'
            if not hk.arg.HK_TARGET then
                hk.arg.HK_TARGET = a;
                table.insert(opt, 'HK_TARGET='..a)
            elseif not hk.arg.HK_CONFIG then
                hk.arg.HK_CONFIG = a;
                table.insert(opt, 'HK_CONFIG='..a)
            elseif not hk.arg.HK_REBUILD then
                hk.arg.HK_REBUILD = a
                table.insert(opt, 'HK_REBUILD='..a)
            end
        else
            table.insert(opt, a)
            table.insert(hk.arg, a)
        end
    elseif k:sub(1,4) ~= 'NDK_' then
        if not hk.arg[k] then
            hk.arg[k] = v --user paramenter
            table.insert(opt, a)
        end
    else
        table.insert(opt, a)
        table.insert(hk.arg, a)
    end
end
if not hk.arg.HK_VERBOSE then
    hk.arg.HK_VERBOSE = 'error'
end

if hk.arg.HK_DRYRUN then
    hk.print('----------------------------------------', 'error')
    for k, v in pairs(hk.arg) do
        hk.print(k ..'='..v, 'error')
    end
    hk.print('----------------------------------------', 'error')
end

return hk
