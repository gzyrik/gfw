local local_path= debug.getinfo(1).source:sub(2)
local local_dir = local_path:match('^(.+)[/\\][^/\\]*$')
package.path = package.path..string.format(";%s/?.lua;%s/?/init.lua",local_dir, local_dir)
---------------------------------------------------
local lmk = require('lmk')

local ret, err = lmk.path.walk('.', {depth=5, file='^lmk%.lua$'}, function (file)
    dofile(file)
    return true
end)
if not ret then return nil, lmk.os.echo(0, err) end

ret,err = lmk.build(arg[1], arg[2])
if not ret then return nil, lmk.os.echo(0, err) end
