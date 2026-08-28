-- surelyramp.lua — reproduce: run right from spawn over the Surely Wood Z2
-- ramp, all the way. Logs every frame; no jumps. We want to see where the
-- player stops or gets bounced back.
local ffi = require('ffi')
local mem = PCSX.getMemPtr()

local ADDR = __PLAYER_DEBUG_ADDR__
local BTN = PCSX.CONSTS.PAD.BUTTON
local pad = PCSX.SIO0.slots[1].pads[1]

local f = 0
local t0 = nil
local log = io.open('/tmp/surelyramp.csv', 'w')

local function r32(off) return ffi.cast('int32_t*', mem + ADDR + off)[0] end
local function r8(off)  return mem[ADDR + off] end
local function revent(off) return r8(off), r32(off + 4), r32(off + 8) end

local function tick()
    local grnd = r32(36)
    if t0 == nil and grnd == 1 then
        t0 = f
        log:write("MARK,t0," .. f .. "\n")
    end

    if t0 ~= nil and (f - t0) > 30 then
        local ang = r32(20)
        -- hold right always after landing
        if ang % 1024 == 0 then pad.setOverride(BTN.RIGHT) end
        pad.setOverride(BTN.RIGHT)
    end

    local g1c, g1x, g1a = revent(72)
    local g2c, g2x, g2a = revent(84)
    local elc, elx, ela = revent(96)
    local erc, erx, era = revent(108)

    -- f, x, y, vx, vy, vz, angle, gsmode, psmode, grnd, push, g1, g2, el, er
    log:write(string.format(
        "CSV,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
        f,
        r32(0), r32(4),
        r32(8), r32(12), r32(16),
        r32(20), r32(24), r32(28),
        grnd, r32(44),
        g1c, g1x >> 12, g1a, g2c, g2x >> 12, g2a,
        elc, elx >> 12, erc, erx >> 12))
end

l = PCSX.Events.createEventListener('GPU::Vsync', function()
    f = f + 1
    local ok, err = pcall(tick)
    if not ok then log:write("ERR," .. tostring(err) .. "\n") end
    if (f % 600) == 0 then log:write("FLUSH," .. f .. "\n"); log:flush() end
end)
log:write(string.format("surelyramp loaded, addr=0x%x\n", ADDR))
