"""Minimal MCP (streamable HTTP) client for PixelLab: init | list | call TOOL JSON_ARGS [OUT.json].
The token is read from scratchpad/px/.token and never printed; long strings (base64) are elided in the output."""
import json, os, ssl, sys, urllib.request

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOKEN = open(os.path.join(BASE, 'px', '.token')).read().strip()
SESSION = os.path.join(BASE, 'px', 'session')
URL = 'https://api.pixellab.ai/mcp'
ctx = ssl.create_default_context(cafile='/root/.ccr/ca-bundle.crt')
opener = urllib.request.build_opener(urllib.request.HTTPSHandler(context=ctx), urllib.request.ProxyHandler())


def post(payload, notify=False):
    headers = {'Content-Type': 'application/json', 'Accept': 'application/json, text/event-stream',
               'Authorization': 'Bearer ' + TOKEN, 'MCP-Protocol-Version': '2025-06-18'}
    if os.path.exists(SESSION):
        headers['Mcp-Session-Id'] = open(SESSION).read().strip()
    req = urllib.request.Request(URL, data=json.dumps(payload).encode(), headers=headers, method='POST')
    try:
        resp = opener.open(req, timeout=600)
    except urllib.error.HTTPError as e:
        body = e.read().decode('utf-8', 'replace')
        sys.exit(f'HTTP {e.code}: {body[:800]}')
    sid = resp.headers.get('Mcp-Session-Id')
    if sid:
        open(SESSION, 'w').write(sid)
    body = resp.read().decode('utf-8', 'replace')
    if notify or not body.strip():
        return None
    if 'text/event-stream' in (resp.headers.get('Content-Type') or ''):
        msgs = [json.loads(l[5:].strip()) for l in body.splitlines() if l.startswith('data:') and l[5:].strip()]
        for m in msgs:
            if m.get('id') == payload.get('id'):
                return m
        return msgs[-1] if msgs else None
    return json.loads(body)


def elide(o):
    if isinstance(o, str):
        return o if len(o) < 600 else o[:120] + f'...<{len(o)} chars>'
    if isinstance(o, list):
        return [elide(v) for v in o]
    if isinstance(o, dict):
        return {k: elide(v) for k, v in o.items()}
    return o


cmd = sys.argv[1]
if cmd == 'init':
    if os.path.exists(SESSION):
        os.remove(SESSION)
    r = post({'jsonrpc': '2.0', 'id': 1, 'method': 'initialize',
              'params': {'protocolVersion': '2025-06-18', 'capabilities': {},
                         'clientInfo': {'name': 'super-clash-dev', 'version': '1.0'}}})
    post({'jsonrpc': '2.0', 'method': 'notifications/initialized'}, notify=True)
    print(json.dumps(elide(r), indent=1)[:3000])
elif cmd == 'list':
    r = post({'jsonrpc': '2.0', 'id': 2, 'method': 'tools/list', 'params': {}})
    out = sys.argv[2] if len(sys.argv) > 2 else None
    if out:
        json.dump(r, open(out, 'w'), indent=1)
    for t in r['result']['tools']:
        print('-', t['name'], ':', (t.get('description') or '').split('\n')[0][:160])
elif cmd == 'call':
    args = json.loads(sys.argv[3])
    r = post({'jsonrpc': '2.0', 'id': 3, 'method': 'tools/call', 'params': {'name': sys.argv[2], 'arguments': args}})
    if len(sys.argv) > 4:
        json.dump(r, open(sys.argv[4], 'w'))
    print(json.dumps(elide(r), indent=1)[:6000])
