#!/usr/bin/env python3
"""Package real JUCE test screenshots into an offline HTML contact sheet. No image generation."""
import argparse
import base64
import hashlib
import html
from pathlib import Path
import shutil
import struct
import zipfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build', type=Path, required=True, help='CTest working directory containing rastro-final-*.png')
parser.add_argument('--out', type=Path, required=True)
args = parser.parse_args()
shots = [
    ('standard', 'Instrumento — tamanho padrão · After Hours · 1180×840'),
    ('compact', 'Instrumento — compacto 1000×700'),
    ('colours', 'Personalizador — CORES'),
    ('knobs', 'Personalizador — KNOBS e prévia interativa'),
    ('grooveiro', 'Personalizador — GROOVEIRO'),
    ('english', 'Personalizador — inglês'),
    ('hidpi', 'Instrumento — renderização 2× (simulada)'),
    ('performance', 'Performance — notas MIDI e PLAY · Sunset Drive'),
    ('cosmic', 'Cosmic Arcade — instrumento e cassete luminoso'),
    ('cosmic-customizer', 'Cosmic Arcade — paleta aplicada no personalizador'),
]
palette_names = ['After Hours', 'Solar Studio', 'Mint Condition', 'Sunrise', 'Sunset Drive', 'Poolside', 'Electric Soul', 'Burnt Velvet']
shots += [(f'palette-{i}', f'Paleta — {name}') for i,name in enumerate(palette_names,1)]
shots += [(f'hardware-{i}', f'Hardware — {name}') for i,name in enumerate(['Marfim canelado', 'Metal escovado', 'Borracha', 'Plástico de console'],1)]
def source_name(key):
    return f'rastro-{key}.png' if key.startswith(('palette-', 'hardware-')) else f'rastro-final-{key}.png'
args.out.mkdir(parents=True, exist_ok=True)
figures, checksums = [], []
for key, title in shots:
    filename = source_name(key)
    source = args.build / filename
    data = source.read_bytes()
    if data[:8] != b'\x89PNG\r\n\x1a\n' or data[12:16] != b'IHDR':
        raise ValueError(f'Invalid PNG: {source}')
    width, height = struct.unpack('>II', data[16:24])
    shutil.copyfile(source, args.out / filename)
    checksums.append(f'{hashlib.sha256(data).hexdigest()}  {filename}')
    figures.append(f'<figure id="{key}"><h2>{html.escape(title)}</h2>'
                   f'<p>{width} × {height} pixels · captura real do teste JUCE</p>'
                   f'<img alt="{html.escape(title)}" src="data:image/png;base64,{base64.b64encode(data).decode()}"></figure>')
page = '''<!doctype html><html lang="pt-BR"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>RASTRO — capturas reais da Etapa 1</title>
<style>body{margin:0;background:#100e1d;color:#f2efff;font:16px system-ui,sans-serif}
main{max-width:1200px;margin:auto;padding:24px}a{color:#a794ff}nav{display:flex;gap:16px;flex-wrap:wrap}
figure{margin:36px 0;padding:16px;background:#201e32;border-radius:12px}h2{font-size:20px}p{color:#c7c0db}
img{width:100%;height:auto;display:block}small{display:block;margin:20px 0}</style>
<main><h1>RASTRO PLAY — Etapa 1 local</h1>
<p>Imagens produzidas pela interface JUCE implementada. Não são mockups ou imagens geradas por IA.
O arquivo visual.wav é um fixture sintético dos testes. Esta revisão não foi publicada nem testada no Logic.</p>
<nav>''' + ''.join(f'<a href="#{key}">{html.escape(title.split("—")[-1].strip())}</a>' for key, title in shots) + '</nav>'
page += ''.join(figures) + '<small>HTML autocontido: abre offline por duplo clique. PNGs originais e checksums acompanham o ZIP.</small></main></html>'
(args.out / 'ABRIR-PREVIAS.html').write_text(page, encoding='utf-8')
(args.out / 'SHA256SUMS.txt').write_text('\n'.join(checksums) + '\n', encoding='utf-8')
(args.out / 'LEIA-ME.txt').write_text('Extraia o ZIP e abra ABRIR-PREVIAS.html por duplo clique. Não exige internet.\n'
                                      'As capturas são reais, feitas no Linux. Não demonstram execução no macOS/Logic.\n', encoding='utf-8')
archive = args.out.parent / 'RASTRO-Etapa1-previas.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as bundle:
    for filename in ['ABRIR-PREVIAS.html', 'SHA256SUMS.txt', 'LEIA-ME.txt'] + [source_name(k) for k, _ in shots]:
        bundle.write(args.out / filename, f'RASTRO-Etapa1/{filename}')
args.out.chmod(0o755)
for exported in args.out.iterdir():
    if exported.is_file():
        exported.chmod(0o644)
archive.chmod(0o644)
print(f'HTML: {args.out / "ABRIR-PREVIAS.html"}')
print(f'ZIP: {archive}')
