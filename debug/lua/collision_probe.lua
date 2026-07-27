-- collision_probe.lua
-- PCSX-Redux Lua probe for real-time 360-degree collision debugging on
-- Sonic XA. Reads the "player_debug" telemetry struct (see include/player.h)
-- and dumps a CSV line per frame, while driving scripted pad input.
--
-- Launch:
--   pcsx-redux -run -interpreter -fastboot -stdout -lua_stdout \
--     -iso build/SONICXA.cue -dofile debug/lua/collision_probe.lua
--
-- PLAYER_DEBUG_ADDR is substituted by debug/run_probe.sh before launching.
-- NOTE: printing every frame crashes this Redux build, so the probe
-- writes the CSV directly to a file via io.open and only prints sparse
-- progress marks.

local ffifail, ffi = pcall(require, 'ffi')

local logfile_ok, logfile = pcall(io.open, '/tmp/collision_probe.csv', 'w')

local function logln(s)
    if logfile_ok and logfile then
        logfile:write(s, "\n")
    else
        print(s)
    end
end

local PLAYER_DEBUG_ADDR = __PLAYER_DEBUG_ADDR__
local mem = PCSX.getMemPtr()

-- Field offsets within PlayerDebugState (see include/player.h)
local OFF = {
    pos_vx = 0, pos_vy = 4,
    vel_vx = 8, vel_vy = 12, vel_vz = 16,
    angle  = 20,
    gsmode = 24, psmode = 28, action = 32,
    grnd   = 36, ceil = 40, push = 44,
    cam_vx = 48, cam_vy = 52,
    raw_g1 = 56, raw_g2 = 60, raw_c1 = 64, raw_c2 = 68,
    ev_grnd1 = 72, ev_grnd2 = 84,
    ev_left = 96, ev_right = 108,
    ev_ceil1 = 120, ev_ceil2 = 132,
}

local function r32(off) return ffifail and ffi.cast('int32_t*', mem + PLAYER_DEBUG_ADDR + off)[0] or 0 end
local function r8(off)  return mem[PLAYER_DEBUG_ADDR + off] end

local function revent(off)
    return r8(off), r32(off + 4), r32(off + 8)
end

-- Pad button constants
local BTN = PCSX.CONSTS.PAD.BUTTON
local pad = PCSX.SIO0.slots[1].pads[1]

local function press(b) pad.setOverride(b) end
local function release(b) pad.clearOverride(b) end
local function release_all()
    release(BTN.RIGHT); release(BTN.LEFT)
end

-- Frame/phase state machine
local frame = -1
local t0 = nil
local PHASE_IDLE, PHASE_RUN = 0, 1

local SAMPLE_N = 4
local RIGHT_START  = 30    -- settle frames after grounding
local RIGHT_FRAMES = 300
local GAP_FRAMES   = 60
local LEFT_FRAMES  = 300
local TAIL_FRAMES  = 90

local LEFT_START  = RIGHT_START + RIGHT_FRAMES + GAP_FRAMES
local DONE_FRAME  = LEFT_START + LEFT_FRAMES + TAIL_FRAMES

local listener

local last_mark_clock = os.clock()
local tick_cost_acc = 0
local tick_cost_n = 0

local function tick()
    local tstart = os.clock()
    frame = frame + 1

    local grnd = r32(OFF.grnd)
    if t0 == nil and grnd == 1 then
        t0 = frame
        logln("MARK,grounded_first_time," .. frame)
    end

    local phase = "PRE"
    local rel = (t0 ~= nil) and (frame - t0) or -1

    if (frame % 300) == 0 then
        if logfile_ok and logfile then logfile:flush() end
        logln(string.format("MARK,alive,%d,pos=(%d,%d)", frame, r32(OFF.pos_vx), r32(OFF.pos_vy)))
    end

    if t0 ~= nil then
        if rel >= RIGHT_START and rel < RIGHT_START + RIGHT_FRAMES then
            press(BTN.RIGHT); phase = "RIGHT"
        elseif rel >= LEFT_START and rel < LEFT_START + LEFT_FRAMES then
            release_all(); press(BTN.LEFT); phase = "LEFT"
        else
            release_all(); phase = "NEUTRAL"
        end

        if rel == DONE_FRAME then
            logln("MARK,done," .. frame)
        end
    end

    -- CSV dump
    local do_sample = ((frame % SAMPLE_N) == 0)
    if do_sample then
    local g1c, g1x, g1a = revent(OFF.ev_grnd1)
    local g2c, g2x, g2a = revent(OFF.ev_grnd2)
    local elc, elx, ela = revent(OFF.ev_left)
    local erc, erx, era = revent(OFF.ev_right)
    local c1c, c1x, c1a = revent(OFF.ev_ceil1)
    local c2c, c2x, c2a = revent(OFF.ev_ceil2)

    logln(string.format(
        "CSV,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
        frame, phase,
        r32(OFF.pos_vx), r32(OFF.pos_vy),
        r32(OFF.vel_vx), r32(OFF.vel_vy), r32(OFF.vel_vz),
        r32(OFF.angle), r32(OFF.gsmode), r32(OFF.psmode),
        grnd, r32(OFF.ceil), r32(OFF.push),
        r32(OFF.raw_g1), r32(OFF.raw_g2),
        g1c, g1x, g1a, g2c, g2x, g2a,
        elc, elx, ela, erc, erx, era,
        c1c, c1x, c1a, c2c, c2a))
    end

    tick_cost_acc = tick_cost_acc + (os.clock() - tstart)
    tick_cost_n = tick_cost_n + 1
    if (frame % 600) == 599 then
        logln(string.format("PERF,frame=%d,tick_avg=%.3fms", frame,
            (tick_cost_acc/math.max(tick_cost_n,1))*1000))
        tick_cost_acc = 0; tick_cost_n = 0
    end
end

local function setup()
    listener = PCSX.Events.createEventListener('GPU::Vsync', function()
        local ok, err = pcall(tick)
        if not ok then logln("ERR," .. tostring(err)) end
    end)
    logln("collision_probe loaded: telemetry phys addr 0x" .. string.format("%08x", PLAYER_DEBUG_ADDR))
    logln("io.open log available: " .. tostring(logfile_ok))
end

setup()
