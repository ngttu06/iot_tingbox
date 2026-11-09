#!/usr/bin/env python3
"""Validate every checked-in audio asset, manifest entry and generated C++ table."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from generate import WORDS

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT/'firmware/assets/audio'
manifest = json.loads((ASSETS/'manifest.json').read_text())
cpp = (ROOT/'firmware/SmartPayESP32/AudioAssets.cpp').read_text()
total = 0
assert {c['id'] for c in manifest['clips']} == set(WORDS)
assert len(manifest['clips']) == len(WORDS)
for c in manifest['clips']:
    path = ASSETS/c['file']
    data = path.read_bytes()
    assert len(data) == c['bytes'], c['id']
    assert hashlib.sha256(data).hexdigest() == c['sha256'], c['id']
    assert c['text'] == WORDS[c['id']], c['id']
    # MPEG-2 Layer III mono, 32 kbps, 22050 Hz; no metadata or reservoir.
    offset = 0
    while offset < len(data):
        frame = data[offset:offset+4]
        assert len(frame) == 4 and frame[:2] == b'\xff\xf3', c['id']
        assert frame[2] >> 4 == 4 and (frame[2] >> 2) & 3 == 0, c['id']
        assert frame[3] >> 6 == 3 and data[offset+4] == 0, c['id']
        offset += 72000 * 32 // 22050 + ((frame[2] >> 1) & 1)
    assert offset == len(data), c['id']
    assert f'"{c["id"]}",clip_{c["id"]}' in cpp, c['id']
    info = json.loads(subprocess.check_output(['ffprobe','-v','error','-show_streams','-of','json',str(path)]))['streams'][0]
    assert info['codec_name'] == 'mp3' and info['channels'] == 1 and info['sample_rate'] == '22050', c['id']
    assert float(info['duration']) > 0.1, c['id']
    subprocess.run(['ffmpeg','-v','error','-xerror','-i',str(path),'-f','null','-'], check=True)
    total += len(data)
print(f'PASS: {len(manifest["clips"])} MP3 clips, mono 22050 Hz, {total} bytes; hashes and embedded table valid')
with tempfile.TemporaryDirectory() as temp:
    for name, ids in {
        'startup': ['chao_mung','ket_noi_wifi'],
        '10000': ['cam_on','da_nhan_duoc','muoi','nghin','viet_nam_dong'],
        '150000': ['cam_on','da_nhan_duoc','mot','tram','nam','muoi_hang_chuc','nghin','viet_nam_dong'],
        'all_clips': list(WORDS),
    }.items():
        joined = Path(temp)/f'{name}.mp3'
        joined.write_bytes(b''.join((ASSETS/f'{id}.mp3').read_bytes() for id in ids))
        subprocess.run(['ffmpeg','-v','error','-xerror','-i',str(joined),'-f','null','-'], check=True)
print('PASS: concatenated announcement streams decode without errors')