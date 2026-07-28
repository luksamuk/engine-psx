-- colprobe3.lua — reproduce: jump to the right into a tall wall with speed.
-- Timeline: spindash right, hold RIGHT; when grounded on the lower right
-- slope with angle in (0x100, 0x400) and good speed, jump once and keep
-- holding RIGHT while logging everything.

local ffi = require('ffi')
local mem = PCSX.getMemPtr()

local ADDR = __PLAYER_DEBUG_ADDR__
local BTN = PCSX.CONSTS.PAD.BUTTON
local pad = PCSX.SIO0.slots[1].pads[1]

local f = 0
local t0 = nil
local log = io.open('/tmp/colprobe3.csv', 'w')
local jumped = 0

local PHASES = { "IDLE", "REV", "RUN", "WATCH", "TAIL" }
local pi = 1
local pf = 0

local function r32(off) return ffi.cast('int32_t*', mem + ADDR + off)[0] end
local function r8(off)  return mem[ADDR + off] end
local function revent(off) return r8(off), r32(off + 4), r32(off + 8) end
local function press(b) pad.setOverride(b) end
local function release(b) pad.clearOverride(b) end
local function clear4() release(BTN.RIGHT); release(BTN.LEFT); release(BTN.DOWN); release(BTN.CROSS) end

local function tick_phases()
    local phase = PHASES[pi]
    local grnd = r32(36)
    local vz = r32(16)
    local avz = (vz < 0) and -vz or vz
    local ang = r32(20)

    clear4()
    if phase == "IDLE" then
        if pf > 60 then pi, pf = pi + 1, 0 end
    elseif phase == "REV" then
        press(BTN.DOWN)
        if (pf % 12) < 5 then press(BTN.CROSS) end
        if pf > 50 then pi, pf = pi + 1, 0 end
    elseif phase == "RUN" then
        press(BTN.RIGHT)
        -- mid-run with speed into the right wall zone: jump!
        if jumped == 0 and grnd == 1 and avz > 0x4000 and r32(0) > 760 * 4096 then
            jumped = 1
            log:write("MARK,jump,", f, ",", r32(0), ",", r32(4), "\n")
            pi, pf = pi + 1, 0
        elseif pf > 1200 then pi, pf = pi + 1, 0 end
    elseif phase == "WATCH" then
        press(BTN.RIGHT)
        if pf < 4 then press(BTN.CROSS) end
        if pf > 300 then pi, pf = pi + 1, 0 end
    else -- TAIL
        if pf > 120 then
            log:write("MARK,done,", f, "\n")
            pi, pf = pi + 1, -100000
        end
    end
    pf = pf + 1
    return PHASES[pi]
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
log:write(string.format("colprobe3 loaded, addr=0x%x\n", ADDR))
