#!/usr/bin/env python3
"""Validate every checked-in audio asset, manifest entry and generated C++ table."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT/'firmware/assets/audio'
manifest = json.loads((ASSETS/'manifest.json').read_text())
cpp = (ROOT/'firmware/SmartPayESP32/AudioAssets.cpp').read_text()
total = 0
for c in manifest['clips']:
    path = ASSETS/c['file']
    data = path.read_bytes()
    assert len(data) == c['bytes'], c['id']
    assert hashlib.sha256(data).hexdigest() == c['sha256'], c['id']
    assert f'"{c["id"]}",clip_{c["id"]}' in cpp, c['id']
    info = json.loads(subprocess.check_output(['ffprobe','-v','error','-show_streams','-of','json',str(path)]))['streams'][0]
    assert info['codec_name'] == 'mp3' and info['channels'] == 1 and info['sample_rate'] == '22050', c['id']
    assert float(info['duration']) > 0.1, c['id']
    subprocess.run(['ffmpeg','-v','error','-xerror','-i',str(path),'-f','null','-'], check=True)
    total += len(data)
print(f'PASS: {len(manifest["clips"])} MP3 clips, mono 22050 Hz, {total} bytes; hashes and embedded table valid')