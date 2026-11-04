"""python3 qa_bases.py: contact sheets (qa_bases_N.png, 4 strips each) of every finished base clip not approved yet."""
import json, os, subprocess
from PIL import Image, ImageDraw
S = 'pxa'
ok = {l.strip() for l in open(S + '/approved.txt')}
todo = []
for f in "asta yuno ichigo byakuya kenpachi ulquiorra grimmjow starrk gojo sukuna link ganondorf kratos hunter".split():
    for c in "jump runstart knockdown holdstart ledgegrab w0guard w1guard".split():
        k = f"{f}/{c}"; jf = f"{S}/{k}/job.json"
        if k in ok or not os.path.exists(jf):
            continue
        if json.load(open(jf)).get('status') == 'done':
            todo.append(k)
ims = []
for k in todo:
    out = f"qa_b_{k.replace('/', '_')}.png"
    subprocess.run(['python3', 'qa_strip.py', out, f'{S}/{k}'], check=True)
    ims.append((k, Image.open(out)))
for old in [f for f in os.listdir('.') if f.startswith('qa_bases_') and f.endswith('.png')]:
    os.remove(old)
for p in range(0, len(ims), 4):
    part = ims[p:p + 4]; W = max(i.width for _, i in part); H = sum(i.height + 14 for _, i in part)
    g = Image.new('RGB', (W, H), (30, 30, 40)); d = ImageDraw.Draw(g); y = 0
    for k, i in part:
        d.text((4, y), k, fill=(255, 255, 0)); g.paste(i, (0, y + 14)); y += i.height + 14
    g.save(f'qa_bases_{p // 4}.png'); print(f'qa_bases_{p // 4}.png', [k for k, _ in part])
