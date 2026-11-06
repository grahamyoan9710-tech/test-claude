"""Plays the game in an Xvfb display and presses keys at given game frames (read from the CLASH_TRACE lines).

    python3 drive.py DISPLAY_NUM SCRIPT GAME_ARGS...
SCRIPT: 'frame:+key' / 'frame:-key' items separated by spaces (press / release), e.g. "240:+j 244:-j".
"""
import ctypes, os, subprocess, sys, time

disp, script, args = sys.argv[1], sys.argv[2], sys.argv[3:]
events = []
for item in script.split():
    fr, act = item.split(':')
    events.append((int(fr), act[0] == '+', act[1:]))
events.sort()

xvfb = subprocess.Popen(['Xvfb', f':{disp}', '-screen', '0', '1600x900x24', '-nolisten', 'tcp'],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(1.5)
x11 = ctypes.CDLL('libX11.so.6')
xtst = ctypes.CDLL('libXtst.so.6')
x11.XOpenDisplay.restype = ctypes.c_void_p
x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
x11.XStringToKeysym.restype = ctypes.c_ulong
x11.XStringToKeysym.argtypes = [ctypes.c_char_p]
x11.XKeysymToKeycode.restype = ctypes.c_ubyte
x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
x11.XFlush.argtypes = [ctypes.c_void_p]
xtst.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
xtst.XTestFakeMotionEvent.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_ulong]
dpy = x11.XOpenDisplay(f':{disp}'.encode())
assert dpy, 'no display'

env = dict(os.environ, DISPLAY=f':{disp}', CLASH_TRACE='1')
game = subprocess.Popen(args, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, bufsize=1)
trace = open(os.environ.get('TRACE_OUT', '/dev/null'), 'w')
i = 0
moved = False
for line in game.stderr:
    if not line.startswith('T '):
        continue
    trace.write(line)
    fr = int(line.split()[1])
    if not moved and fr > 5:
        xtst.XTestFakeMotionEvent(dpy, -1, 640, 360, 0)
        x11.XFlush(dpy)
        moved = True
    while i < len(events) and events[i][0] <= fr:
        _, down, key = events[i]
        kc = x11.XKeysymToKeycode(dpy, x11.XStringToKeysym(key.encode()))
        xtst.XTestFakeKeyEvent(dpy, kc, 1 if down else 0, 0)
        x11.XFlush(dpy)
        i += 1
game.wait()
trace.close()
xvfb.terminate()
print('done, exit', game.returncode)
