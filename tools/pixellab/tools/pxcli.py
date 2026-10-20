#!/usr/bin/env python3
"""PixelLab jobs for Super Clash Bros., safe against double billing.

Every job lives in its own directory OUT: the submission is written to OUT/job.json BEFORE anything else, and a
directory that already has a job.json is never submitted again (the command waits for that job instead). So a
crashed or repeated command never pays twice.

  pxcli.py balance
  pxcli.py upload FILE.png                      -> prints the upload ref (upload:<id>, valid 3 days)
  pxcli.py anim OUT --src FILE|REF --desc TEXT [--frames 8] [--engine mm|classic] [--last FILE|REF]
                    [--subject TEXT] [--pose TEXT] [--direction east] [--seed N]
  pxcli.py image OUT --desc TEXT --w 128 --h 64 [--direction east] [--view side] [--init FILE --strength 300]
  pxcli.py wait OUT [--timeout 900]             -> waits for OUT/job.json, downloads frames to OUT/f_NN.png
  pxcli.py jobs                                 -> running jobs

engine mm = animate_image_pixminimax (1-2 generations, 4-40 frames, multiple of 4); classic = animate_image
(frames x 1 generation at 256x256, 4-8 frames, even). Frames come back as OUT/f_00.png (the input, unchanged)
then f_01.. the generated ones; OUT/result.json keeps the reply.
"""
import argparse, base64, fcntl, json, os, ssl, subprocess, sys, time, urllib.error, urllib.request

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOKEN = open(os.path.join(BASE, 'px', '.token')).read().strip()
SESSION = os.path.join(BASE, 'px', 'session_cli')
LOCK = os.path.join(BASE, 'px', 'session_cli.lock')
URL = 'https://api.pixellab.ai/mcp'
CTX = ssl.create_default_context(cafile='/root/.ccr/ca-bundle.crt')
OPENER = urllib.request.build_opener(urllib.request.HTTPSHandler(context=CTX), urllib.request.ProxyHandler())


def _post(payload, sid=None, notify=False):
    headers = {'Content-Type': 'application/json', 'Accept': 'application/json, text/event-stream',
               'Authorization': 'Bearer ' + TOKEN, 'MCP-Protocol-Version': '2025-06-18'}
    if sid:
        headers['Mcp-Session-Id'] = sid
    req = urllib.request.Request(URL, data=json.dumps(payload).encode(), headers=headers, method='POST')
    resp = OPENER.open(req, timeout=300)
    new_sid = resp.headers.get('Mcp-Session-Id')
    body = resp.read().decode('utf-8', 'replace')
    if notify or not body.strip():
        return None, new_sid
    if 'text/event-stream' in (resp.headers.get('Content-Type') or ''):
        msgs = [json.loads(l[5:].strip()) for l in body.splitlines() if l.startswith('data:') and l[5:].strip()]
        for m in msgs:
            if m.get('id') == payload.get('id'):
                return m, new_sid
        return (msgs[-1] if msgs else None), new_sid
    return json.loads(body), new_sid


def _session(fresh=False):
    with open(LOCK, 'w') as lk:
        fcntl.flock(lk, fcntl.LOCK_EX)
        if not fresh and os.path.exists(SESSION):
            return open(SESSION).read().strip()
        r, sid = _post({'jsonrpc': '2.0', 'id': 1, 'method': 'initialize',
                        'params': {'protocolVersion': '2025-06-18', 'capabilities': {},
                                   'clientInfo': {'name': 'super-clash-cli', 'version': '1.0'}}})
        _post({'jsonrpc': '2.0', 'method': 'notifications/initialized'}, sid, notify=True)
        open(SESSION, 'w').write(sid or '')
        return sid or ''


def call(tool, args, retries=4):
    """Calls a tool; returns (text, structuredContent, raw). Retries on network errors and expired sessions."""
    last = None
    for attempt in range(retries):
        try:
            sid = _session(fresh=attempt > 0 and isinstance(last, urllib.error.HTTPError))
            r, _ = _post({'jsonrpc': '2.0', 'id': 3, 'method': 'tools/call', 'params': {'name': tool, 'arguments': args}}, sid)
            if r is None:
                raise RuntimeError('empty reply')
            if 'error' in r:
                raise RuntimeError(json.dumps(r['error'])[:800])
            res = r['result']
            text = '\n'.join(c.get('text', '') for c in res.get('content', []) if c.get('type') == 'text')
            return text, res.get('structuredContent') or {}, r
        except urllib.error.HTTPError as e:
            last = e
            body = e.read().decode('utf-8', 'replace')[:400]
            if e.code in (400, 404) and 'session' in body.lower():
                continue
            if e.code >= 500 or e.code == 429:
                time.sleep(5 * (attempt + 1))
                continue
            raise RuntimeError(f'HTTP {e.code}: {body}')
        except (urllib.error.URLError, TimeoutError, ConnectionError) as e:
            last = e
            time.sleep(5 * (attempt + 1))
    raise RuntimeError(f'{tool} failed after {retries} tries: {last}')


def upload(path):
    for attempt in range(6):  # an upload is free: a TLS reset (curl 35) just asks for a new URL and another try
        text, sc, _ = call('get_upload_url', {})
        url = sc.get('upload_url') or next(w for w in text.split() if w.startswith('https://api.pixellab.ai/v2/uploads/'))
        r = subprocess.run(['curl', '-sS', '--cacert', '/root/.ccr/ca-bundle.crt', '-F', f'file=@{path}', url],
                           capture_output=True, text=True)
        if r.returncode == 0:
            try:
                return json.loads(r.stdout)['ref']
            except (ValueError, KeyError):
                pass
        time.sleep(4 * (attempt + 1))
    raise RuntimeError(f'upload of {path} failed')


def ref_of(x):
    return x if x.startswith(('upload:', 'https://', 'gallery:')) else upload(x)


def job_id_of(text, sc):
    jid = sc.get('job_id') or sc.get('id')
    if not jid:
        for line in text.splitlines():
            if line.startswith('job_id:') or line.startswith('id:'):
                jid = line.split(':', 1)[1].strip()
                break
    return jid


def submit(out, tool, args, meta):
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, '.lock'), 'w') as lk:  # one submitter per folder, whatever runs in parallel
        fcntl.flock(lk, fcntl.LOCK_EX)
        return _submit(out, tool, args, meta)


SCRATCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def _known_ids():
    import glob
    ids = set()
    for jf in glob.glob(os.path.join(SCRATCH, '**', 'job.json'), recursive=True):
        try:
            jid = json.load(open(jf)).get('job_id')
        except (ValueError, OSError):
            continue
        if jid:
            ids.add(jid)
    return ids


def _young_unknown_jobs(max_age=150):
    """Jobs PixelLab runs that no job.json records, started in the last max_age seconds."""
    import re
    text = call('list_jobs', {})[0]
    known = _known_ids()
    found = []
    for line in text.splitlines():
        m = re.match(r'([0-9a-f-]{36}) .*\((?:\w+), (?:(\d+)m)?(\d+)s old\)', line)
        if m and m.group(1) not in known and int(m.group(2) or 0) * 60 + int(m.group(3)) <= max_age:
            found.append(m.group(1))
    return found


def _submit(out, tool, args, meta):
    jf = os.path.join(out, 'job.json')
    if os.path.exists(jf) and json.load(open(jf)).get('status') != 'rejected':
        print('already submitted:', json.load(open(jf)).get('job_id'), '(not resubmitted)')
        return
    # Reserve the directory first: a crash between the call and the write must not lead to a second submission.
    json.dump({'status': 'submitting', 'tool': tool, 'args': args, **meta}, open(jf, 'w'))
    glk = open(os.path.join(SCRATCH, 'px', 'submit.lock'), 'w')
    for attempt in range(400):
        # One submission at a time across every process, so a job PixelLab started without telling us (the
        # connection dropped after the call) can be told apart from the others and adopted instead of paid twice.
        fcntl.flock(glk, fcntl.LOCK_EX)
        try:
            text, sc, _ = call(tool, args, retries=1)
        except RuntimeError as e:  # an explicit refusal (busy, bad argument) is not billed: wait and retry
            msg = str(e)
            if 'urlopen error' in msg or 'Connection reset' in msg or 'timed out' in msg.lower():
                time.sleep(10)
                orphans = _young_unknown_jobs()
                if len(orphans) == 1:
                    json.dump({'status': 'submitted', 'job_id': orphans[0], 'adopted': True, 'tool': tool, 'args': args,
                               **meta}, open(jf, 'w'), indent=1)
                    fcntl.flock(glk, fcntl.LOCK_UN)
                    print('submitted', orphans[0], '(adopted after a dropped connection)')
                    return
                fcntl.flock(glk, fcntl.LOCK_UN)
                if not orphans and attempt < 399:
                    time.sleep(5)
                    continue
                json.dump({'status': 'ambiguous' if orphans else 'rejected', 'error': msg[:800], 'orphans': orphans,
                           'tool': tool, 'args': args, **meta}, open(jf, 'w'), indent=1)
                raise
            fcntl.flock(glk, fcntl.LOCK_UN)
            busy = any(w in msg.lower() for w in ('concurren', 'too many', 'rate limit', 'busy', 'slot', '429'))
            if busy and attempt < 399:
                time.sleep(30)
                continue
            if msg.startswith('HTTP 4') or 'isError' in msg or busy or 'error' in msg.lower():
                json.dump({'status': 'rejected', 'error': msg[:800], 'tool': tool, 'args': args, **meta}, open(jf, 'w'), indent=1)
            raise
        low = text.lower()
        if not job_id_of(text, sc) and any(w in low for w in ('concurren', 'too many', 'rate limit', 'busy')):
            fcntl.flock(glk, fcntl.LOCK_UN)
            time.sleep(30)
            continue
        break
    jid = job_id_of(text, sc)
    if not jid:
        json.dump({'status': 'rejected', 'error': text[:800], 'tool': tool, 'args': args, **meta}, open(jf, 'w'), indent=1)
        fcntl.flock(glk, fcntl.LOCK_UN)
        sys.exit('rejected: ' + text[:400])
    json.dump({'status': 'submitted', 'job_id': jid, 'tool': tool, 'args': args, 'reply': text, **meta}, open(jf, 'w'), indent=1)
    fcntl.flock(glk, fcntl.LOCK_UN)
    print('submitted', jid)
    print(text[:600])


def gallery_frames(asset, out):
    """Writes the frames of a saved Creator image as OUT/f_NN.png and returns how many. The pixel tools give them as
    one sheet of frames, row by row, from api.pixellab.ai: the storage hosts of the download links are not reachable
    from every network (Claude cloud sessions)."""
    import io, re
    from PIL import Image
    for attempt in range(8):
        text, _, _ = call('pixelart_workbench', {'argv': ['inspect', 'gallery:' + asset], 'mode': 'low'})
        img = re.search(r'^image_id: (\S+)', text, re.M)
        size = re.search(r'canvas: (\d+)x(\d+), (\d+) frame', text)
        if img and size:
            break
        time.sleep(5 + 5 * attempt)  # "the pixel tools are busy"
    else:
        raise RuntimeError(f'gallery:{asset}: no frames ({text[:200]})')
    fw, fh, n = (int(v) for v in size.groups())
    req = urllib.request.Request(f'{URL}/pixel-tools/{img.group(1)}/frames.png',
                                 headers={'Authorization': 'Bearer ' + TOKEN})
    sheet = Image.open(io.BytesIO(OPENER.open(req, timeout=300).read())).convert('RGBA')
    cols = sheet.width // fw
    for i in range(n):
        x, y = (i % cols) * fw, (i // cols) * fh
        sheet.crop((x, y, x + fw, y + fh)).save(os.path.join(out, f'f_{i:02d}.png'))
    return n


def wait(out, timeout=1200):
    jf = os.path.join(out, 'job.json')
    job = json.load(open(jf))
    jid = job.get('job_id')
    if not jid:
        sys.exit(f'{out}: no job id recorded (status {job.get("status")}); look at the PixelLab gallery before resubmitting')
    t0 = time.time()
    while True:
        text, sc, raw = call('get_image', {'job_id': jid})
        status = (sc.get('status') or '').lower() or ('completed' if 'status: completed' in text else
                                                      'failed' if 'status: failed' in text or 'error' in text.lower()[:200] else 'processing')
        asset = sc.get('asset_id') or next((l.split(':', 1)[1].strip() for l in text.splitlines()
                                            if l.startswith('asset_id:')), None)
        if 'completed' in status and not asset:  # an old job: get_image no longer gives it, the submission did
            asset = next((l.split(':', 1)[1].strip() for l in job.get('reply', '').splitlines()
                          if l.startswith('asset_id:')), None)
        if 'completed' in status and not asset and 'asset_status:' in text and time.time() - t0 < timeout:
            time.sleep(10)  # done, but not saved to the gallery yet
            continue
        if 'completed' in status:
            json.dump(raw, open(os.path.join(out, 'result.json'), 'w'))
            if asset:
                urls = [None] * gallery_frames(asset, out)
            else:
                urls = sc.get('download_urls') or []
            if not urls:
                base = next((w for w in text.split() if '/download?index=0' in w), None)
                n = int(next((l.split(':')[1] for l in text.splitlines() if l.startswith('frames:')), '1'))
                urls = [base.replace('index=0', f'index={i}') for i in range(n)] if base else []
            for i, u in enumerate(urls):
                if u is None:
                    continue  # already written from the gallery
                for attempt in range(8):  # TLS hiccups through the proxy: try again
                    r = subprocess.run(['curl', '-sS', '--retry', '4', '--cacert', '/root/.ccr/ca-bundle.crt', '-o',
                                        os.path.join(out, f'f_{i:02d}.png'), u])
                    if r.returncode == 0:
                        break
                    time.sleep(3 + 3 * attempt)
                else:
                    raise RuntimeError(f'download failed: {u}')
            job['status'] = 'done'
            job['frames'] = len(urls)
            job['charged'] = next((l.split(':', 1)[1].strip() for l in text.splitlines() if l.startswith('charged:')), '')
            json.dump(job, open(jf, 'w'), indent=1)
            print(f'done {jid}: {len(urls)} frames -> {out} ({job["charged"]})')
            return True
        if 'fail' in status:
            job['status'] = 'failed'
            job['error'] = text[:800]
            json.dump(job, open(jf, 'w'), indent=1)
            print('FAILED', jid, text[:800])
            return False
        if time.time() - t0 > timeout:
            print('still processing after', timeout, 's:', jid)
            return False
        time.sleep(20)


def submitted(out):
    jf = os.path.join(out, 'job.json')
    return os.path.exists(jf) and json.load(open(jf)).get('status') != 'rejected'


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest='cmd', required=True)
    sub.add_parser('balance')
    sub.add_parser('jobs')
    u = sub.add_parser('upload'); u.add_argument('file')
    a = sub.add_parser('anim'); a.add_argument('out'); a.add_argument('--src', required=True); a.add_argument('--desc', required=True)
    a.add_argument('--frames', type=int, default=8); a.add_argument('--engine', default='mm', choices=['mm', 'classic'])
    a.add_argument('--last'); a.add_argument('--subject'); a.add_argument('--pose'); a.add_argument('--direction', default='east')
    a.add_argument('--seed', type=int); a.add_argument('--no-wait', action='store_true')
    i = sub.add_parser('image'); i.add_argument('out'); i.add_argument('--desc', required=True)
    i.add_argument('--w', type=int, default=128); i.add_argument('--h', type=int, default=64)
    i.add_argument('--direction'); i.add_argument('--view', default='side'); i.add_argument('--init'); i.add_argument('--strength', type=int, default=300)
    i.add_argument('--no-wait', action='store_true'); i.add_argument('--model', default='pixflux', choices=['pixflux', 'pixen', 'pro'])
    i.add_argument('--outline'); i.add_argument('--detail')
    i.add_argument('--ref', action='append', default=[], help='PATH|usage: a labelled reference picture (pro model)')
    w = sub.add_parser('wait'); w.add_argument('out'); w.add_argument('--timeout', type=int, default=1200)
    a_ = ap.parse_args()
    if a_.cmd == 'balance':
        print(call('get_balance', {})[0])
    elif a_.cmd == 'jobs':
        print(call('list_jobs', {})[0])
    elif a_.cmd == 'upload':
        print(upload(a_.file))
    elif a_.cmd == 'anim':
        if submitted(a_.out):
            print('already submitted, waiting')
        else:
            args = {'first_frame_url': ref_of(a_.src)}
            if a_.last:
                args['last_frame_url'] = ref_of(a_.last)
            if a_.seed is not None:
                args['seed'] = a_.seed
            if a_.engine == 'mm':
                tool = 'animate_image_pixminimax'
                args.update({'description': a_.desc, 'frame_count': a_.frames, 'view': 'side', 'direction': a_.direction})
                if a_.subject: args['subject_description'] = a_.subject
                if a_.pose: args['initial_pose'] = a_.pose
            else:
                tool = 'animate_image'
                args.update({'action': a_.desc, 'frame_count': a_.frames})
            submit(a_.out, tool, args, {'src': a_.src, 'last': a_.last})
        if not a_.no_wait:
            sys.exit(0 if wait(a_.out) else 1)
    elif a_.cmd == 'image':
        if not submitted(a_.out):
            args = {'description': a_.desc, 'width': a_.w, 'height': a_.h, 'no_background': True}
            if a_.model != 'pro':
                args['view'] = a_.view
                if a_.direction: args['direction'] = a_.direction
                if a_.outline: args['outline'] = a_.outline
                if a_.detail: args['detail'] = a_.detail
            if a_.init:
                args['init_image_base64'] = base64.b64encode(open(a_.init, 'rb').read()).decode()
                args['init_image_strength'] = a_.strength
            if a_.ref:
                args['reference_images'] = [{'base64': base64.b64encode(open(r.split('|', 1)[0], 'rb').read()).decode(),
                                             'usage': r.split('|', 1)[1] if '|' in r else 'reference'} for r in a_.ref]
            tool = {'pixflux': 'create_image_pixflux', 'pixen': 'create_image_pixen', 'pro': 'create_image_pro'}[a_.model]
            submit(a_.out, tool, args, {})
        if not a_.no_wait:
            sys.exit(0 if wait(a_.out) else 1)
    elif a_.cmd == 'wait':
        sys.exit(0 if wait(a_.out, a_.timeout) else 1)


if __name__ == '__main__':
    main()
