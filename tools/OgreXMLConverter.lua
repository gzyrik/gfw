#!/usr/bin/env lua
local local_path= debug.getinfo(1).source:sub(2)
local local_dir = local_path:match('^(.+)[/\\][^/\\]*$')
package.path = package.path..string.format(";%s/?.lua;%s/?/init.lua",local_dir, local_dir)
---------------------------------------------------
local table = require("lmk.table")
---------------------------------------------------
local _indent={newline=true, depth=0, space='    '}
local function script_begin(key, name)
    if not _indent.newline then
        io.write('\n')
    end
    for i=1,_indent.depth do
        io.write(_indent.space)
    end
    if name then
        io.write(':', key, ' ',name, '(\n')
    else
        io.write(':', key, '(\n')
    end
    _indent.newline = true
    _indent.depth = _indent.depth + 1
end
local function script_end()
    io.write(')')
    _indent.newline = nil
    _indent.depth = _indent.depth - 1
end
local function script_write(key, ...)
    if not key then return end
    if not _indent.newline then
        io.write('\n')
    end
    for i=1,_indent.depth do
        io.write(_indent.space)
    end
    if select('#', ...) == 1 then
        io.write(':', key, ' ', ...)
    else
        io.write(':', key, ' ', '[', table.concat({...}, ' '), ']')
    end
    _indent.newline = nil
end
local function format_number(num)
    if math.ceil(num) ~= tonumber(num) then
        return string.format('%.6f', num)
    else
        return num
    end
end
local function script_write_quat(key, item)
    local half=item.xarg.angle/2;
    local sinf=math.sin(half)
    local conf=math.cos(half)
    for _, axis in ipairs(item) do
        if axis.label == 'axis' then
            script_write(key,
            format_number(sinf*axis.xarg.x),
            format_number(sinf*axis.xarg.y),
            format_number(sinf*axis.xarg.z),
            format_number(conf))
            break
        end
    end
end
---------------------------------------------------
local function submesh_names(names)
    local ret={}
    for _,i in ipairs(names) do
        if i.label == 'submeshname' then
            table.insert(ret, tonumber(i.xarg.index)+1, i.xarg.name)
        end
    end
    return ret
end
local function idecl_script_put(isize, count)
    script_begin('declare')
    script_write('isize', isize)
    script_write('count', count)
    script_end()
end
local function indices_script_put(faces)
    script_begin('indices')
    for _, item in ipairs(faces) do
        if item.label == 'face' then
            script_write('face', item.xarg.v1, item.xarg.v2, item.xarg.v3)
        end
    end
    script_end()
end
local function vdecl_script_put(geometry)
    script_begin('declare')
    local source = 0
    for _, item in ipairs(geometry) do
        if item.label == 'vertexbuffer' then
            if item.xarg.positions == 'true' then
                script_begin('element')
                script_write('semantic', 'position')
                script_write('type', 'float3')
                script_write('source', source)
                script_end()
            end
            if item.xarg.normals == 'true' then
                script_begin('element')
                script_write('semantic', 'normal')
                script_write('type', 'float3')
                script_write('source', source)
                script_end()
            end
            if item.xarg.texture_coords then
                local ntex = tonumber(item.xarg.texture_coords)-1
                for i=0,ntex do
                    local ftex = item.xarg['texture_coord_dimensions_'..i];
                    if ftex then
                        script_begin('element')
                        script_write('semantic', 'texcoord')
                        script_write('type', 'float'..ftex)
                        script_write('index', i)
                        script_write('source', source)
                        script_end()
                    end
                end
            end
            source = source + 1
        end
    end
    script_end()
end
local _maxBounds= {-100000, -100000, -100000}
local _minBounds= { 100000,  100000,  100000}
local function pos(v, i)
    if v then
        local x = tonumber(v)
        if x < _minBounds[i] then _minBounds[i] = x end
        if x > _maxBounds[i] then _maxBounds[i] = x end
    end
    return v
end
local function bounds_script_put()
    script_begin('bounds')
    script_write('minimum', _minBounds[1], _minBounds[2], _minBounds[3])
    script_write('maximum', _maxBounds[1], _maxBounds[2], _maxBounds[3])
    script_end()
end
local function vertex_script_put(vertex)
    script_begin('vertex')
    for _, item in ipairs(vertex) do
        if item.label == 'position'  then
            script_write('position', pos(item.xarg.x,1), pos(item.xarg.y,2), pos(item.xarg.z,3))
        elseif item.label == 'normal' then
            script_write('normal', item.xarg.x, item.xarg.y, item.xarg.z)
        elseif item.label == 'texcoord' then
            script_write('texcoord', item.xarg.u, item.xarg.v, item.xarg.w, item.xarg.x)
        end
    end
    script_end()
end
local function vbuf_script_put(vbuf, i)
    script_begin('source', i)
    for _, item in ipairs(vbuf) do
        if item.label == 'vertex' then
            vertex_script_put(item)
        end
    end
    script_end()
end
local function geometry_script_put(geometry)
    local i=0
    script_begin('geometry')
    script_write('count', geometry.xarg.vertexcount)
    vdecl_script_put (geometry)
    for _, item in ipairs(geometry) do
        if item.label == 'vertexbuffer' then
            vbuf_script_put (item, i)
            i = i + 1
        end
    end
    script_end()
end
local function bassign_script_put(bassign)
    script_begin("boneassignments");
    for _, item in ipairs(bassign) do
        if item.label == 'vertexboneassignment' then
            script_write('assign', item.xarg.vertexindex, item.xarg.boneindex, item.xarg.weight)
        end
    end
    script_end()
end
local function get_primitive(primitive, faces)
    if primitive == 'triangle_list' then
        return 'triangles', tonumber(faces)*3
    end
end
local function submesh_script_put(submesh, name)
    script_begin('submesh', name)
    script_write('material', submesh.xarg.material)

    if submesh.xarg.usesharedvertices == 'true' then
        script_write('shared_vdata', submesh.xarg.usesharedvertices)
    end

    for _, item in ipairs(submesh) do
        if item.label == 'faces' then
            local isize = submesh.xarg.use32bitindexes == 'true' and 4 or  2
            local primitive, count = get_primitive (submesh.xarg.operationtype, item.xarg.count)
            script_write('primitive', primitive)
            idecl_script_put (isize, count)
            indices_script_put(item)
        elseif item.label == 'geometry' then
            geometry_script_put(item)
        elseif item.label == 'boneassignments' then
            bassign_script_put(item)
        end
    end
    script_end()
end
local function mesh_script_put(mesh, name)
    local submeshes, subnames
    local i=1
    script_begin('mesh', name)

    for _, item in ipairs(mesh) do
        if item.label == 'submeshnames' then
            subnames = submesh_names(item)
        elseif item.label == 'submeshes' then
            submeshes = item
        elseif item.label == 'sharedgeometry' then
            geometry_script_put(item)
        elseif item.label == 'skeletonlink' then
            script_write('skeleton', item.xarg.name)
        end
    end
    for _,item in ipairs(submeshes) do
        if item.label == 'submesh' then
            submesh_script_put(item, subnames and subnames[i] or nil)
            i = i + 1
        end
    end
    bounds_script_put()
    script_end()
end
---------------------------------------------------
local _bones={}
local function bone_script_put(bone, id)
    script_begin('bone', id)
    if bone.xarg.name then _bones[bone.xarg.name] = id end
    for _, item in ipairs(bone) do
        if item.label == 'position' then
            script_write('position', item.xarg.x, item.xarg.y, item.xarg.z)
        elseif item.label == 'rotation' then
            script_write_quat('rotation', item)
        end
    end
    script_end()
end
local function bones_script_put(bones)
    local id = 0
    script_begin('bones', #bones)
    for _, item in ipairs(bones) do
        if item.label == 'bone' then
            bone_script_put(item, item.xarg.id or id)
            if not item.xarg.id then id = id + 1 end
        end
    end
    script_end()
end
local function hierarchy_script_put(hierarchy)
    script_begin('hierarchy')
    for _, item in ipairs(hierarchy) do
        if item.label == 'boneparent' then
            script_write('parent', _bones[item.xarg.bone], _bones[item.xarg.parent])
        end
    end
    script_end()
end
local function keyframe_script_put(keyframe)
    script_begin('keyframe', keyframe.xarg.time)
    for _, item in ipairs(keyframe) do
        if item.label == 'translate'  then
            script_write('translate', pos(item.xarg.x,1), pos(item.xarg.y,2), pos(item.xarg.z,3))
        elseif item.label == 'rotate' then
            script_write_quat('rotate', item)
        end
    end
    script_end()
end
local function keyframes_script_put(keyframes)
    for _, item in ipairs(keyframes) do
        if item.label == 'keyframe' then
            keyframe_script_put(item)
        end
    end
end
local function track_script_put(track)
    script_begin('track', _bones[track.xarg.bone])
    for _, item in ipairs(track) do
        if item.label == 'keyframes' then
            keyframes_script_put(item)
        end
    end
    script_end()
end
local function tracks_script_put(tracks)
    for _, item in ipairs(tracks) do
        if item.label == 'track' then
            track_script_put(item)
        end
    end
end
local function animation_script_put(animation)
    script_begin('animation', animation.xarg.name)
    script_write('length', animation.xarg.length)
    for _, item in ipairs(animation) do
        if item.label == 'tracks' then
            tracks_script_put(item)
        end
    end
    script_end()
end
local function animations_script_put(animations)
    for _, item in ipairs(animations) do
        if item.label == 'animation' then
            animation_script_put(item)
        end
    end
end
local function skeleton_script_put(skeleton, name)
    script_begin('skeleton', name)
    for _, item in ipairs(skeleton) do
        if item.label == 'bones' then
            bones_script_put(item)
        elseif item.label == 'bonehierarchy' then
            hierarchy_script_put(item)
        elseif item.label == 'animations' then
            animations_script_put(item)
        end
    end
    script_end()
end

local name = string.match(arg[1],'^(.-%.mesh)%.xml$') or string.match(arg[1],'^(.-%.skeleton)%.xml$')
if name then
    io.input(arg[1])
    for i, item in ipairs(table.xml(io.read("*a"))) do
        if item.label == 'mesh' then
            mesh_script_put(item, item.xarg.name or name)
        elseif item.label == 'skeleton' then
            skeleton_script_put(item, item.xarg.name or name)
        end
    end
end
