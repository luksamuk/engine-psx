-- colprobe2.lua — lightweight collision probe (works at ~90fps).
-- Logs full telemetry CSV to /tmp/colprobe2.csv and drives scripted input.
--
-- Test sequence (all relative to first grounded frame t0):
--   IDLE   [t0+0,   t0+60[    -- settle
--   REV1   [t0+60,  t0+110[   -- spindash: hold DOWN, tap CROSS
--   RUN1   [t0+110, t0+520[   -- release DOWN (dash fires right), hold RIGHT
--   GAP    [t0+520, t0+580[   -- neutral
--   TURN   [t0+580, t0+597[   -- tap LEFT briefly, settle (face left at rest)
--   REV2   [t0+597, t0+647[   -- spindash again
--   RUN2   [t0+647, t0+1007[  -- release DOWN (dash fires left), hold LEFT
--   TAIL   from t0+1007       -- neutral
--   DONE   t0+1150            -- mark end

local ffi = require('ffi')
local mem = PCSX.getMemPtr()

local ADDR = __PLAYER_DEBUG_ADDR__
local BTN = PCSX.CONSTS.PAD.BUTTON
local pad = PCSX.SIO0.slots[1].pads[1]

local f = 0
local t0 = nil
local log = io.open('/tmp/colprobe2.csv', 'w')

local function r32(off) return ffi.cast('int32_t*', mem + ADDR + off)[0] end
local function r8(off)  return mem[ADDR + off] end
local function revent(off) return r8(off), r32(off + 4), r32(off + 8) end

local function press(b) pad.setOverride(b) end
local function release(b) pad.clearOverride(b) end
local function clear4()
    release(BTN.RIGHT); release(BTN.LEFT); release(BTN.DOWN); release(BTN.CROSS)
end
local function r32(off) return ffi.cast('int32_t*', mem + ADDR + off)[0] end

-- Gated phase machine with robust stop detection.
local PHASES = { "IDLE", "REV1", "RUN1", "JUMP", "TAIL" }
local pi = 1
local pf = 0        -- frames inside the current phase
local still = 0     -- consecutive frames grounded and nearly stopped

local function curphase() return PHASES[pi] end

local STOP_VZ = 0x100       -- 0.25 px/frame in 20.12? no: 0x100 = 1/16 px... generous
local STILL_NEED = 90       -- ~1.5s still before allowing actions

local function tick_phases()
    local phase = curphase()
    local grnd = r32(36)
    local vz = r32(16)
    local avz = (vz < 0) and -vz or vz

    if grnd == 1 and avz < STOP_VZ then
        still = still + 1
    else
        still = 0
    end

    clear4()
    if phase == "IDLE" then
        if pf > 60 then pi, pf = pi + 1, 0 end
    elseif phase == "REV1" then
        press(BTN.DOWN)
        if (pf % 12) < 5 then press(BTN.CROSS) end
        if pf > 50 then pi, pf = pi + 1, 0 end
    elseif phase == "RUN1" then
        press(BTN.RIGHT)
        -- when moving fast enough to the right, jump while keeping RIGHT
        if pf > 140 and pf < 165 then press(BTN.CROSS) end
        if pf > 340 then pi, pf = pi + 1, 0 end
    else -- TAIL
        if pf > 400 then
            log:write("MARK,done,", f, "\n")
            pi, pf = pi + 1, -100000
        end
    end
    pf = pf + 1
    return curphase()
end

local function tick()
    local grnd = r32(36)
    if t0 == nil and grnd == 1 then
        t0 = f
        log:write("MARK,t0,", f, "\n")
    end

    local phase = "PRE"
    if t0 ~= nil then
        phase = tick_phases()
    end

    local g1c, g1x, g1a = revent(72)
    local g2c, g2x, g2a = revent(84)
    local elc, elx, ela = revent(96)
    local erc, erx, era = revent(108)
    local c1c, c1x, c1a = revent(120)
    local c2c, c2x, c2a = revent(132)

    log:write(string.format(
        "CSV,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
        f, phase,
        r32(0), r32(4),
        r32(8), r32(12), r32(16),
        r32(20), r32(24), r32(28), r32(32),
        grnd, r32(40), r32(44),
        r32(56), r32(60),
        g1c, g1x, g1a, g2c, g2x, g2a,
        elc, elx, ela, erc, erx, era,
        c1c, c1x, c1a, c2c, c2x, c2a))
end

l = PCSX.Events.createEventListener('GPU::Vsync', function()
    f = f + 1
    local ok, err = pcall(tick)
    if not ok then log:write("ERR," .. tostring(err) .. "\n") end
    if (f % 600) == 0 then log:write("FLUSH,", f, "\n"); log:flush() end
end)
log:write(string.format("colprobe2 loaded, addr=0x%x\n", ADDR))
