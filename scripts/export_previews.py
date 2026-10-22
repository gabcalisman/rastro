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
parser.add_argument('--before', type=Path, help='Optional historical screenshots: before-interface.png and before-character.png')
args = parser.parse_args()
shots = [
    ('standard', 'Instrumento — tamanho padrão · 1100×760'),
    ('compact', 'Instrumento — compacto 1000×700'),
    ('colours', 'Personalizador — CORES'),
    ('knobs', 'Personalizador — KNOBS e prévia interativa'),
    ('character', 'Personalizador — PERSONAGEM'),
    ('english', 'Personalizador — inglês'),
    ('hidpi', 'Instrumento — renderização 2× (simulada)'),
    ('performance', 'Performance — notas MIDI e PLAY · Sunset Drive'),
    ('cosmic', 'Cosmic Arcade — instrumento e cassete luminoso'),
    ('cosmic-customizer', 'Cosmic Arcade — paleta aplicada no personalizador'),
]
shots += [('synth', 'SYNTH — ADSR, filtro, modulação e LOOP'), ('patches', 'PATCHES — busca real e comparação A/B'), ('patches-compact', 'PATCHES — compacto'), ('capture', 'CAPTURAR — receber tomada do insert')]
shots += [('adsr-compact', 'ADSR — quatro knobs e gráfico no compacto'),
          ('capture-recording', 'CAPTURAR — gravando pelo PLAY'),
          ('capture-ready', 'CAPTURAR — WAV finalizado e selecionado'),
          ('capture-loaded', 'CAPTURAR — tomada carregada na waveform'),
          ('capture-insert', 'RASTRO Capture — painel do insert gravando')]
shots += [(f'character-presentation-{i}', f'Personagem — apresentação {i}') for i in range(3)]
shots += [(f'character-dance-{i}', f'Dança real — quadro {i}') for i in range(8)]
shots += [(f'character-{light}-{i}', f'Personagem — {light} · apresentação {i}')
          for light in ['dark', 'light'] for i in range(3)]
shots += [(f'character-small-{palette}-{i}', f'Personagem — pequeno · paleta {palette} · apresentação {i}')
          for palette in [1, 2] for i in range(3)]
palette_names = ['After Hours', 'Solar Studio', 'Mint Condition', 'Sunrise', 'Sunset Drive', 'Poolside', 'Electric Soul', 'Burnt Velvet']
shots += [(f'palette-{i}', f'Paleta — {name}') for i,name in enumerate(palette_names,1)]
shots += [(f'hardware-{i}', f'Hardware — {name}') for i,name in enumerate(['Marfim canelado', 'Metal escovado', 'Borracha', 'Plástico de console'],1)]
if args.before:
    shots += [('before-interface', 'Antes — interface publicada no commit 234af84'),
              ('before-character', 'Antes — personagem publicado no commit 234af84')]
def source_name(key):
    special={'synth':'rastro-instrument-panel.png','patches':'rastro-patch-panel.png','patches-compact':'rastro-patch-compact.png','capture':'rastro-capture-receiver.png'}
    if key in special: return special[key]
    if key.startswith('before-'): return f'{key}.png'
    if key.startswith(('character-', 'capture-')) or key == 'adsr-compact': return f'rastro-{key}.png'
    return f'rastro-{key}.png' if key.startswith(('palette-', 'hardware-')) else f'rastro-final-{key}.png'
def output_name(key):
    aliases = {'standard':'instrumento', 'compact':'compacto', 'colours':'cores',
               'character':'personagem', 'cosmic-customizer':'cosmic-personalizador',
               'patches-compact':'patches-compacto', 'capture':'captura'}
    return f'{aliases.get(key, key).replace("palette-", "paleta-")}.png'
args.out.mkdir(parents=True, exist_ok=True)
figures, checksums = [], []
for key, title in shots:
    filename = source_name(key)
    source = (args.before if key.startswith('before-') else args.build) / filename
    data = source.read_bytes()
    if data[:8] != b'\x89PNG\r\n\x1a\n' or data[12:16] != b'IHDR':
        raise ValueError(f'Invalid PNG: {source}')
    width, height = struct.unpack('>II', data[16:24])
    filename = output_name(key)
    if source.resolve() != (args.out / filename).resolve():
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
<main><h1>RASTRO PLAY — capturas reais</h1>
<p>Imagens produzidas pela interface JUCE implementada. Não são mockups ou imagens geradas por IA.
O arquivo visual.wav é um fixture sintético dos testes. Capturas no Linux; não demonstram teste no Logic.</p>
<nav>''' + ''.join(f'<a href="#{key}">{html.escape(title.split("—")[-1].strip())}</a>' for key, title in shots) + '</nav>'
page += ''.join(figures) + '<small>HTML autocontido: abre offline por duplo clique. PNGs originais e checksums acompanham o ZIP.</small></main></html>'
(args.out / 'ABRIR-PREVIAS.html').write_text(page, encoding='utf-8')
(args.out / 'SHA256SUMS.txt').write_text('\n'.join(checksums) + '\n', encoding='utf-8')
(args.out / 'LEIA-ME.txt').write_text('Extraia o ZIP e abra ABRIR-PREVIAS.html por duplo clique. Não exige internet.\n'
                                      'As capturas são reais, feitas no Linux. Não demonstram execução no macOS/Logic.\n', encoding='utf-8')
archive = args.out.parent / 'RASTRO-PLAY-previas.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as bundle:
    for filename in ['ABRIR-PREVIAS.html', 'SHA256SUMS.txt', 'LEIA-ME.txt'] + [output_name(k) for k, _ in shots]:
        bundle.write(args.out / filename, f'RASTRO-PLAY/{filename}')
args.out.chmod(0o755)
for exported in args.out.iterdir():
    if exported.is_file():
        exported.chmod(0o644)
archive.chmod(0o644)
print(f'HTML: {args.out / "ABRIR-PREVIAS.html"}')
print(f'ZIP: {archive}')
