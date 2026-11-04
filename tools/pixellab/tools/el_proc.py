"""Trims silence, normalizes the peak and encodes mono MP3 for the game."""
import os, subprocess, sys
import numpy as np
src, dst = sys.argv[1], sys.argv[2]
R = 44100
for f in sorted(os.listdir(src)):
    if not f.endswith('.mp3'): continue
    pcm = subprocess.run(['ffmpeg', '-v', 'error', '-i', os.path.join(src, f), '-ac', '1', '-ar', str(R), '-f', 's16le', '-'],
                         capture_output=True, check=True).stdout
    x = np.frombuffer(pcm, np.int16).astype(np.float32) / 32768.0
    thr = 10 ** (-45 / 20) * np.abs(x).max()
    idx = np.nonzero(np.abs(x) > thr)[0]
    if len(idx): x = x[max(0, idx[0] - 50): idx[-1] + int(0.03 * R)]
    x = x / max(1e-6, np.abs(x).max()) * 0.89
    n = min(len(x), int(0.02 * R)); x[-n:] *= np.linspace(1, 0, n)
    raw = (x * 32767).astype(np.int16).tobytes()
    subprocess.run(['ffmpeg', '-v', 'error', '-y', '-f', 's16le', '-ar', str(R), '-ac', '1', '-i', '-', '-b:a', '96k',
                    os.path.join(dst, f)], input=raw, check=True)
print('ok', len(os.listdir(dst)))
