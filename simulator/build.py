"""Build the native Windows simulator using the self-contained Zig C++ compiler."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
HERE = ROOT / 'simulator'
LIB = ROOT / 'lib/ProtoTracer'
OUT = HERE / 'build'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--run', action='store_true')
    args = parser.parse_args()
    if os.name != 'nt':
        raise SystemExit('This target uses the Windows desktop API; build it on Windows.')
    try:
        import ziglang
    except ImportError:
        raise SystemExit(f'Install the compiler first: "{sys.executable}" -m pip install ziglang==0.16.0')
    OUT.mkdir(exist_ok=True)
    sources = []
    for folder in ('Animation', 'Camera', 'Engine', 'Renderer', 'Scene', 'Utils', 'Physics'):
        sources.extend(p for p in (LIB / folder).rglob('*.cpp')
                       if p.name != 'Clock.cpp')
    for file in (
        'Controller/Controller.cpp', 'Controller/HUB75Controller.cpp',
        'Examples/Templates/Project.cpp', 'Examples/Templates/ProtogenProjectTemplate.cpp',
        'Examples/Templates/ProtogenHUB75Update.cpp',
        'ExternalDevices/InputDevices/ButtonHandler.cpp',
        'ExternalDevices/InputDevices/Menu/Menu.cpp',
        'ExternalDevices/InputDevices/Menu/Movies.cpp',
        'ExternalDevices/InputDevices/Menu/TextScroller.cpp',
        'ExternalDevices/Displays/SSD1306.cpp',
        'ExternalDevices/Sensors/APDS9960.cpp',
        'ExternalDevices/Sensors/Microphone/Utils/MicrophoneFourierBase.cpp',
        'ExternalDevices/OutputDevices/FanController.cpp',
        'Assets/Models/FBX/Utils/Morph.cpp',
        'Assets/Textures/Animated/Utils/ImageSequence.cpp',
    ):
        sources.append(LIB / file)
    sources.extend((HERE / 'main.cpp', HERE / 'runtime.cpp', HERE / 'microphone.cpp'))
    command = [sys.executable, '-m', 'ziglang', 'c++', '-std=c++17', '-O2',
               '-DPROJECT_PROTOGEN_HUB75', '-DPROTOTRACER_SIMULATOR', '-DNOMINMAX',
               '-Wno-unused-value', '-Wno-return-stack-address', '-Wno-nullability-completeness',
               '-Wno-inconsistent-missing-override', '-Wno-vla-cxx-extension', '-Wno-unused-command-line-argument',
               '-Wl,--subsystem,windows',
               '-I' + str(HERE / 'compat'), '-I' + str(LIB),
               *map(str, sources), '-o', str(OUT / 'ProtoTracerSimulator.exe'),
               '-luser32', '-lgdi32', '-lwinmm']
    result = subprocess.run(command, cwd=OUT)
    if result.returncode:
        raise SystemExit(result.returncode)
    print(OUT / 'ProtoTracerSimulator.exe')
    if args.run:
        subprocess.Popen([str(OUT / 'ProtoTracerSimulator.exe')], cwd=OUT)


if __name__ == '__main__':
    main()
