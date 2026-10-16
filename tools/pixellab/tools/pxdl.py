"""Downloads the frames of a PixelLab get_image result (structuredContent.download_urls) into a new directory."""
import json, os, subprocess, sys
res, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=False)
urls = json.load(open(res))['result']['structuredContent']['download_urls']
for i, u in enumerate(urls):
    subprocess.run(['curl', '-sS', '-o', os.path.join(out, f'f_{i:02d}.png'), u], check=True)
print(len(urls), 'frames ->', out)
