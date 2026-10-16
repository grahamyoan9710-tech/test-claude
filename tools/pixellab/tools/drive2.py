"""Runs the game in its own Xvfb and presses keys at given times: 'secs:+key' / 'secs:-key'; 'secs:key' = tap."""
import ctypes, os, subprocess, sys, time
disp, script, args = sys.argv[1], sys.argv[2], sys.argv[3:]
ev = []
for it in script.split():
    t, a = it.split(':', 1)
    t = float(t)
    if a[0] == '@': ev.append((t, None, a[1:]))
    elif a[0] == '#': ev.append((t, 'click', a[1:]))  # '#x,y': move the pointer there and click
    elif a[0] in '+-': ev.append((t, a[0] == '+', a[1:]))
    else: ev += [(t, True, a), (t + 0.3, False, a)]
ev.sort(key=lambda e: e[0])
xvfb = subprocess.Popen(['Xvfb', f':{disp}', '-screen', '0', '1600x900x24', '-nolisten', 'tcp'],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(1.5)
x11 = ctypes.CDLL('libX11.so.6'); xt = ctypes.CDLL('libXtst.so.6')
x11.XOpenDisplay.restype = ctypes.c_void_p; x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
x11.XStringToKeysym.restype = ctypes.c_ulong; x11.XStringToKeysym.argtypes = [ctypes.c_char_p]
x11.XKeysymToKeycode.restype = ctypes.c_ubyte; x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
x11.XFlush.argtypes = [ctypes.c_void_p]
xt.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
xt.XTestFakeMotionEvent.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_ulong]
xt.XTestFakeButtonEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
dpy = x11.XOpenDisplay(f':{disp}'.encode())
game = subprocess.Popen(args, env=dict(os.environ, DISPLAY=f':{disp}'), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
t0 = time.time()
time.sleep(0.5)
xt.XTestFakeMotionEvent(dpy, -1, 5, 5, 0); x11.XFlush(dpy)
for t, down, key in ev:
    dt = t - (time.time() - t0)
    if dt > 0: time.sleep(dt)
    if down == 'click':
        x, y = (int(v) for v in key.split(','))
        xt.XTestFakeMotionEvent(dpy, -1, x, y, 0); x11.XFlush(dpy); time.sleep(0.4)
        xt.XTestFakeButtonEvent(dpy, 1, 1, 0); x11.XFlush(dpy); time.sleep(0.3)
        xt.XTestFakeButtonEvent(dpy, 1, 0, 0); x11.XFlush(dpy)
        continue
    if down is None:
        subprocess.run(['import', '-display', f':{disp}', '-window', 'root', key], check=False)
        continue
    kc = x11.XKeysymToKeycode(dpy, x11.XStringToKeysym(key.encode()))
    xt.XTestFakeKeyEvent(dpy, kc, 1 if down else 0, 0); x11.XFlush(dpy)
game.wait(); xvfb.terminate(); print('done', game.returncode)
