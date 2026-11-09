#!/usr/bin/env python3
"""Generate original synthetic Vietnamese clips with eSpeak NG, not macOS licensed voices."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'firmware/assets/audio'
WORDS = {
    'da_nhan': 'đã nhận', 'khong': 'không', 'mot': 'một', 'hai': 'hai',
    'ba': 'ba', 'bon': 'bốn', 'nam': 'năm', 'sau': 'sáu', 'bay': 'bảy',
    'tam': 'tám', 'chin': 'chín', 'muoi': 'mười', 'muoi_hang_chuc': 'mươi',
    'tram': 'trăm', 'nghin': 'nghìn', 'trieu': 'triệu', 'ty': 'tỷ',
    'linh': 'linh', 'mot_sau_muoi': 'mốt', 'lam': 'lăm', 'dong': 'đồng'
}

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    clips = []
    with tempfile.TemporaryDirectory() as temp:
        for name, text in WORDS.items():
            wav = Path(temp) / f'{name}.wav'
            mp3 = OUT / f'{name}.mp3'
            subprocess.run(['espeak-ng', '-v', 'vi', '-s', '145', '-p', '45', '-a', '100', '-w', str(wav), text], check=True)
            subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', str(wav), '-af',
                            'silenceremove=start_periods=1:start_threshold=-45dB:stop_periods=1:stop_threshold=-45dB,loudnorm=I=-20:TP=-3:LRA=7,apad=pad_dur=0.04',
                            '-ac', '1', '-ar', '22050', '-codec:a', 'libmp3lame', '-b:a', '32k',
                            '-map_metadata', '-1', str(mp3)], check=True)
            data = mp3.read_bytes()
            clips.append({'id': name, 'text': text, 'file': mp3.name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
    manifest = {'engine': 'eSpeak NG', 'engine_version': subprocess.check_output(['espeak-ng','--version'], text=True).splitlines()[0],
                'voice': 'vi (Northern Vietnamese), synthetic male', 'rate': 145,
                'format': 'MP3 mono 22050 Hz 32 kbps', 'clips': clips}
    (OUT/'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+'\n')
    subprocess.run(['python3', str(ROOT/'tools/audio/embed.py')], check=True)
    print(f'Generated {len(clips)} clips, {sum(c["bytes"] for c in clips)} bytes')

if __name__ == '__main__':
    main()