#!/usr/bin/env python3
"""Generate clips on macOS using Linh; ESP32 only plays embedded MP3 bytes."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'firmware/assets/audio'
WORDS = {
    'cam_on': 'Cảm ơn', 'da_nhan_duoc': 'đã nhận được',
    'khong': 'không', 'mot': 'một', 'hai': 'hai',
    'ba': 'ba', 'bon': 'bốn', 'nam': 'năm', 'sau': 'sáu', 'bay': 'bảy',
    'tam': 'tám', 'chin': 'chín', 'muoi': 'mười', 'muoi_hang_chuc': 'mươi',
    'tram': 'trăm', 'nghin': 'nghìn', 'trieu': 'triệu', 'ty': 'tỷ',
    'linh': 'linh', 'mot_sau_muoi': 'mốt', 'lam': 'lăm', 'viet_nam_dong': 'Việt Nam đồng'
}

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    clips = []
    with tempfile.TemporaryDirectory() as temp:
        for name, text in WORDS.items():
            wav = Path(temp) / f'{name}.aiff'
            mp3 = OUT / f'{name}.mp3'
            subprocess.run(['/usr/bin/say', '-v', 'Linh', '-r', '185', '-o', str(wav), text], check=True)
            pad = '0.08' if name == 'cam_on' else '0.025'
            # Trim only edges, preserving pauses inside phrases and weak consonants.
            filters = ('silenceremove=start_periods=1:start_threshold=-50dB,'
                       'areverse,silenceremove=start_periods=1:start_threshold=-50dB,'
                       'areverse,loudnorm=I=-18:TP=-2:LRA=7,' + f'apad=pad_dur={pad}')
            subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', str(wav), '-af',
                            filters,
                            '-ac', '1', '-ar', '22050', '-codec:a', 'libmp3lame', '-b:a', '32k',
                            '-write_xing', '0', '-id3v2_version', '0', '-reservoir', '0',
                            '-map_metadata', '-1', str(mp3)], check=True)
            data = mp3.read_bytes()
            clips.append({'id': name, 'text': text, 'file': mp3.name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
    manifest = {'engine': 'macOS say', 'engine_version': subprocess.check_output(['sw_vers','-productVersion'], text=True).strip(),
                'voice': 'Linh (vi_VN)', 'rate': 185,
                'stream': 'MPEG frames only; no ID3/Xing, bit reservoir disabled',
                'format': 'MP3 mono 22050 Hz 32 kbps', 'clips': clips}
    (OUT/'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+'\n')
    for obsolete in ('da_nhan.mp3', 'dong.mp3'):
        (OUT/obsolete).unlink(missing_ok=True)
    subprocess.run(['python3', str(ROOT/'tools/audio/embed.py')], check=True)
    print(f'Generated {len(clips)} clips, {sum(c["bytes"] for c in clips)} bytes')

if __name__ == '__main__':
    main()