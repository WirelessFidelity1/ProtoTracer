"""Run with py tools/test_face_editor.py (opens a hidden Tk window)."""
import tempfile
import tkinter as tk
from pathlib import Path
from face_editor import Model, Editor, MODELS


def test():
    for name in ('NukudeFlat.h', 'NukudeFace.h', 'GammaFace.h'):
        model = Model(MODELS / name)
        assert model.render() == model.source
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / name
            path.write_bytes(model.source.encode('utf-8'))
            edited = Model(path)
            edited.arrays['basisVertices'][0][0] += 3
            morph = next(iter(edited.morphs))
            ids, vectors = edited.morphs[morph]
            edited.arrays[vectors][0][1] += 2
            expected = edited.vertices(morph)
            backup = edited.save()
            assert backup.read_bytes() == model.source.encode('utf-8')
            loaded = Model(path)
            for actual, wanted in zip(loaded.vertices(morph), expected):
                assert all(abs(a-b) < 0.000002 for a, b in zip(actual, wanted))
            assert loaded.arrays['basisIndexes'] == model.arrays['basisIndexes']
            assert loaded.arrays[ids] == model.arrays[ids]
            # A second save exercises span recalculation.
            edited.arrays['basisVertices'][1][2] += 1
            edited.save()
            assert Model(path).arrays['basisVertices'][1][2] == edited.arrays['basisVertices'][1][2]
            path.write_bytes(b'external change')
            try:
                edited.save()
            except ValueError:
                pass
            else:
                raise AssertionError('External edits were not protected')
    root = tk.Tk()
    root.withdraw()
    ui = Editor(root)
    assert ui.model.path.name == 'NukudeFlat.h'
    root.update()
    ui.selected = {0}
    before = ui.model.vertices('Base shape')[0][:]
    ui.snapshot()
    ui.shift([1, 2, 3])
    assert ui.model.vertices('Base shape')[0] == [v+d for v, d in zip(before, [1, 2, 3])]
    ui.undo()
    assert not ui.dirty()
    ui.mode.set('Anger')
    ui.change_mode()
    ui.selected = {ui.editable()[0]}
    base = ui.model.arrays['basisVertices'][next(iter(ui.selected))][:]
    ui.shift([2, 0, 0])
    assert ui.model.arrays['basisVertices'][next(iter(ui.selected))] == base
    for plane in ('XY', 'XZ', 'YZ'):
        ui.plane.set(plane)
        ui.fit()
        root.update()
    root.destroy()
    print('PASS: mesh/morph round trips, backups, external edit protection, UI, undo, views')


if __name__ == '__main__':
    test()
