"""Calls one PixelLab MCP tool and prints its text: python3 pxcall.py TOOL 'JSON args' [--save DIR]
Images in the reply (base64) are saved to DIR as call_N.png when --save is given. Never prints the token."""
import base64, json, os, sys
sys.argv, args = [sys.argv[0]], sys.argv[1:]
import importlib.util
spec = importlib.util.spec_from_file_location('px', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pxcli.py'))
px = importlib.util.module_from_spec(spec); spec.loader.exec_module(px)
tool, a = args[0], json.loads(args[1]) if len(args) > 1 else {}
save = args[args.index('--save') + 1] if '--save' in args else None
text, sc, raw = px.call(tool, a)
print(text)
if save:
    os.makedirs(save, exist_ok=True)
    n = 0
    for c in raw['result'].get('content', []):
        if c.get('type') == 'image':
            n += 1
            open(os.path.join(save, f'call_{n}.png'), 'wb').write(base64.b64decode(c['data']))
    if n:
        print(f'[{n} image(s) saved to {save}]')
