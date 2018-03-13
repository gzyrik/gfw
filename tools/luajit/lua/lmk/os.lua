local lfs = require("lfs")
local _os= os
_os.arg = {}
_os.verbose = 8
_os.cd = lfs.chdir
_os.cwd = lfs.currentdir
-----------------------------------------------------
--- 0 error
--- 2 warning
--- 4 title
--- 6 info
--- 8 debug
-----------------------------------------------------
-- 内部实用函数
-----------------------------------------------------
-- 打印函数
local verbose_color={
    '\027[1;40;31m',
    '\027[1;40;33m',
    '\027[1;40;34m',
    '\027[1;40;36m',
    '\027[1;40;32m',
}
function _os.echo(level, ...)
    if _os.verbose >= level then
        if _os.clicolor then
            io.write('lmk>', verbose_color[math.floor(level/2) + 1], ...)
            io.write('\027[0m\n')
        else
            io.write('lmk>', ...)
            io.write('\n')
        end
    end
    return table.concat({...})
end
-----------------------------------------------------------
function _os.system(cmd)
    _os.echo(8,cmd)
    if not _os.dryrun then
        if not os.execute(cmd) then return nil, 'error: '..cmd end
    end
    return true
end
-----------------------------------------------------------
local function uname()
    local f = os.getenv('OS')
    if f then return string.lower(f) end

    f = io.popen('uname')
    if not f then return "windows" end

    local n = f:read('*a')
    f:close()
    if n then 
        return string.lower(n:match('%w*')) 
    end
    return 'windows'
end
_os.name = uname()
_os.clicolor = not _os.name:find("windows")
-----------------------------------------------------
-- 解析命令行参数
local cwd='.'
for _,a in ipairs(arg) do
    local k, v = string.match(a, '^(.-)=(.+)$')
    if k then
        if k:sub(1,2) == '--' then
            if k == '--verbose' then
                _os.verbose = tonumber(v)
            elseif k == '--clicolor' then
                _os.clicolor = v
            elseif k == '--dryrun' then
                _os.dryrun = v
            elseif k == '--directory' then
                cwd = v
            end
        else
            _os.arg[k] = v
        end
    end
end
_os.echo(4, 'os=', _os.name, '\tcwd=', cwd)
_os.cd(cwd)
-----------------------------------------------------
return _os
