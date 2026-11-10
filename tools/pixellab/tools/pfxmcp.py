# ParticleFX Studio MCP client (streamable HTTP), the owner's account:
#   python3 tools/pixellab/tools/pfxmcp.py tools/list
#   python3 tools/pixellab/tools/pfxmcp.py tools/call '{"name":"list_effects","arguments":{}}' [IMAGE_DIR]
# The key is in tools/pixellab/pfx/.key (ignored by git; never print or commit it). create_effect, build_effect and
# export_effect cost builds: only with the owner's go.
import json, sys, urllib.request
URL = 'https://mcp.particlefx.studio/mcp'
KEY = open(__import__('os').path.join(__import__('os').path.dirname(__file__), '..', 'pfx', '.key')).read().strip()
def post(body, sid=None):
    h = {'Authorization': 'Bearer ' + KEY, 'Content-Type': 'application/json', 'Accept': 'application/json, text/event-stream'}
    if sid: h['Mcp-Session-Id'] = sid
    r = urllib.request.urlopen(urllib.request.Request(URL, json.dumps(body).encode(), h), timeout=900)  # an agent build takes minutes
    raw = r.read().decode()
    sid = r.headers.get('Mcp-Session-Id') or sid
    data = [l[5:] for l in raw.splitlines() if l.startswith('data:')]
    if data:  # an event stream (a long build sends progress events first): the reply is the last event
        raw = data[-1]
    try:
        return (json.loads(raw) if raw.strip() else None), sid
    except ValueError:
        sys.exit('not JSON: ' + raw[:400])
res, sid = post({'jsonrpc': '2.0', 'id': 1, 'method': 'initialize', 'params': {'protocolVersion': '2025-03-26', 'capabilities': {}, 'clientInfo': {'name': 'clash-horizon', 'version': '1'}}})
print('server:', json.dumps(res.get('result', res).get('serverInfo', res))[:300])
post({'jsonrpc': '2.0', 'method': 'notifications/initialized'}, sid)
method = sys.argv[1] if len(sys.argv) > 1 else 'tools/list'
params = json.loads(sys.argv[2]) if len(sys.argv) > 2 else {}
res, _ = post({'jsonrpc': '2.0', 'id': 2, 'method': method, 'params': params}, sid)
out = sys.argv[3] if len(sys.argv) > 3 else None  # a folder: images in the reply (previews) are saved there, not printed
for i, c in enumerate((res or {}).get('result', {}).get('content', [])):
    if c.get('type') == 'image' and out:
        __import__('os').makedirs(out, exist_ok=True)
        f = __import__('os').path.join(out, f'{i}.png')
        open(f, 'wb').write(__import__('base64').b64decode(c.pop('data')))
        c['saved'] = f
print(json.dumps(res, ensure_ascii=False, indent=1))
