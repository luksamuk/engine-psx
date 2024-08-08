if TilesetMode == nil then return app.alert "Use Aseprite v1.3"  end

-- Export tilesets for aseprite files with two layers: foreground and background
-- Layer names don't matter. Make sure foreground is on top.
-- 1. Copy both layers.
-- 2. Turn original layers invisible
-- 3. Flatten copied layers, make them visible
-- 4. Convert flattened layer to tilemap layer
-- 5. Export tileset
-- 6. Delete flattened layer

--local row_len = 32

local function activeFrameNumber()
  local f = app.activeFrame
  if f == nil then
    return 1
  else
    return f
  end
end


app.command.GotoFirstFrame()
local spr = app.activeSprite
local lay = app.activeLayer
local fs = app.fs
local output_folder = fs.filePath(spr.filename)
if not lay.isTilemap then return app.alert "No active tilemap layer" end

local function layers_tilemap_duplicate(spr)
  for i,layer in ipairs(spr.layers) do
    if layer.isTilemap then
      app.activeLayer = layer
      layer.isVisible = true    
      app.command.DuplicateLayer()
      layer.isVisible = false
    else
      layer.isVisible = false
    end
  end
end

local function layers_visible_flatten(spr)
  for i,layer in ipairs(spr.layers) do
    app.command.FlattenLayers{["visibleOnly"]="true"}
    local grid = spr.gridBounds
    grid.height = 8
    grid.width = 8
    spr.gridBounds = grid

    --row_len = 256 / grid.width
  end
end

local function layers_2visible(spr)
  for i,layer in ipairs(spr.layers) do
      layer.isVisible = true
  end
end

local function layers_visible2tilemap(spr)
  for i,layer in ipairs(spr.layers) do
    if layer.isVisible then
      app.command.ConvertLayer{["to"]="tilemap"}
    end
  end
end


local function layers_getName(layers,name)
  local out
  for i,layer in ipairs(layers) do
    if (layers[i].name  == name) then
      out = layers[i]
      break
    end
  end
  return out
end

layers_tilemap_duplicate(spr)
layers_visible_flatten(spr)
layers_visible2tilemap(spr)

local lay = layers_getName(spr.layers,"Flattened")
app.activeLayer = lay
if lay.isTilemap then
  local tileset = lay.tileset
  local spec = spr.spec
  local grid = tileset.grid
  local size = grid.tileSize
  local row_len = 256 / size.width
  spec.width = 256
  local col_len = math.ceil(#tileset / row_len)
  spec.height = size.height *  col_len
  local image = Image(spec)
  image:clear()

  for i = 0,row_len-1 do
    for j = 0, col_len do
      local current = i + j* row_len
      if (current < #tileset) then
        local tile = tileset:getTile(current)
        image:drawImage(tile, i*size.width, j*size.height)
      end
    end
  end
  local path = fs.joinPath(output_folder, app.fs.fileTitle(spr.filename) .. ".png")
  image:saveAs(path)
end
app.command.RemoveLayer()
layers_2visible(spr)
