"""Small, offline mesh/morph editor for ProtoTracer's generated C++ headers."""
from pathlib import Path
import copy
import math
import re
import shutil
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from datetime import datetime

ROOT = Path(__file__).resolve().parents[1]
MODELS = ROOT / 'lib/ProtoTracer/Assets/Models/FBX'
ARRAY = re.compile(r'\b(Vector3D|IndexGroup|int)\s+(\w+)\s*\[\s*(\d+)\s*\]\s*=\s*\{([^{}]*)\}', re.S)


class Model:
    def __init__(self, path):
        self.path = Path(path)
        self.source = self.path.read_bytes().decode('utf-8')
        self.arrays, self.spans = {}, {}
        for m in ARRAY.finditer(self.source):
            kind, name, count, body = m.groups()
            if kind == 'int':
                values = [int(v.strip()) for v in body.split(',') if v.strip()]
            else:
                values = [[float(x.strip().rstrip('fF')) for x in args.split(',')]
                          for args in re.findall(kind + r'\s*\(([^)]+)\)', body)]
                if any(len(v) != 3 or not all(math.isfinite(x) for x in v) for v in values):
                    raise ValueError('Invalid coordinates: ' + name)
            if len(values) != int(count):
                raise ValueError('Unsupported array: ' + name)
            self.arrays[name] = values
            self.spans[name] = (m.start(4), m.end(4), kind)
        if not self.arrays.get('basisVertices') or not self.arrays.get('basisIndexes'):
            raise ValueError('This header has no supported face mesh.')
        self.morphs = {}
        for count, indexes, vectors in re.findall(r'Morph\(\s*(\d+)\s*,\s*(\w+)\s*,\s*(\w+)\s*\)', self.source):
            ids, vecs = self.arrays[indexes], self.arrays[vectors]
            if len(ids) != int(count) or len(vecs) != int(count):
                raise ValueError('Morph count mismatch')
            self.morphs[vectors.removesuffix('Vectors')] = (indexes, vectors)
        n = len(self.arrays['basisVertices'])
        if any(int(i) != i or i < 0 or i >= n for tri in self.arrays['basisIndexes'] for i in tri):
            raise ValueError('Invalid triangle index')
        if any(i < 0 or i >= n for ids, _ in self.morphs.values() for i in self.arrays[ids]):
            raise ValueError('Invalid morph index')
        self.original = copy.deepcopy(self.arrays)

    def vertices(self, morph, weight=1):
        result = copy.deepcopy(self.arrays['basisVertices'])
        if morph in self.morphs:
            ids, vecs = self.morphs[morph]
            for i, delta in zip(self.arrays[ids], self.arrays[vecs]):
                result[i] = [a + weight*b for a, b in zip(result[i], delta)]
        return result

    def render(self):
        result = self.source
        for name, (start, end, kind) in sorted(self.spans.items(), key=lambda item: item[1][0], reverse=True):
            if self.arrays[name] == self.original[name]:
                continue
            if kind != 'Vector3D':
                raise ValueError('Topology editing is unsupported')
            values = self.arrays[name]
            if any(not math.isfinite(x) for v in values for x in v):
                raise ValueError('Coordinates must be finite')
            body = ','.join('Vector3D(' + ','.join(f'{x:.6f}f' for x in v) + ')' for v in values)
            result = result[:start] + body + result[end:]
        return result

    def save(self):
        if self.path.read_bytes().decode('utf-8') != self.source:
            raise ValueError('The header changed outside the editor. Reopen it before saving.')
        result = self.render()
        if result == self.source:
            return None
        backup = self.path.with_name(self.path.name + '.' + datetime.now().strftime('%Y%m%d-%H%M%S-%f') + '.bak')
        shutil.copy2(self.path, backup)
        self.path.write_bytes(result.encode('utf-8'))
        self.source = result
        # Recompute spans after replacement lengths change.
        fresh = Model(self.path)
        self.spans, self.original = fresh.spans, fresh.original
        return backup


class Editor:
    def __init__(self, root):
        self.root = root
        self.model = None
        self.history = []
        self.selected = set()
        self.drag = None
        root.title('ProtoTracer Face Editor')
        root.geometry('1120x760')
        bar = ttk.Frame(root, padding=8)
        bar.pack(fill='x')
        for label, action in [('Open header', self.open), ('Save + backup', self.save), ('Undo', self.undo), ('Fit view', self.fit)]:
            ttk.Button(bar, text=label, command=action).pack(side='left', padx=3)
        self.mode = tk.StringVar(value='Base shape')
        self.modes = ttk.Combobox(bar, textvariable=self.mode, state='readonly', width=24)
        self.modes.pack(side='left', padx=10)
        self.modes.bind('<<ComboboxSelected>>', self.change_mode)
        self.plane = tk.StringVar(value='XY')
        planes = ttk.Combobox(bar, textvariable=self.plane, values=['XY', 'XZ', 'YZ'], state='readonly', width=4)
        planes.pack(side='left')
        planes.bind('<<ComboboxSelected>>', lambda e: self.fit())
        body = ttk.Frame(root)
        body.pack(fill='both', expand=True)
        self.canvas = tk.Canvas(body, background='#111b25', highlightthickness=0)
        self.canvas.pack(side='left', fill='both', expand=True)
        side = ttk.Frame(body, padding=14, width=220)
        side.pack(side='right', fill='y')
        ttk.Label(side, text='Expression preview').pack(anchor='w')
        self.weight = tk.DoubleVar(value=1)
        ttk.Scale(side, from_=0, to=1, variable=self.weight, command=lambda _: self.draw()).pack(fill='x')
        ttk.Label(side, text='Drag edits require preview = 100%.\nBase edits affect all expressions.', wraplength=210).pack(pady=8)
        self.wire = tk.BooleanVar(value=True)
        ttk.Checkbutton(side, text='Triangle edges', variable=self.wire, command=self.draw).pack(anchor='w')
        self.labels = tk.BooleanVar(value=False)
        ttk.Checkbutton(side, text='Vertex numbers', variable=self.labels, command=self.draw).pack(anchor='w')
        self.info = ttk.Label(side, text='No selection')
        self.info.pack(pady=16)
        self.coords = []
        for axis in 'XYZ':
            row = ttk.Frame(side)
            row.pack(fill='x', pady=3)
            ttk.Label(row, text=axis, width=3).pack(side='left')
            var = tk.StringVar()
            ttk.Entry(row, textvariable=var, width=16).pack(side='left')
            self.coords.append(var)
        ttk.Button(side, text='Set selected point', command=self.set_point).pack(pady=8)
        ttk.Label(side, text='Click a point to select.\nShift-click adds/removes points.\nDrag to move the selection.\nMouse wheel zooms.\nRight-drag pans.\nCtrl+Z undoes; Ctrl+S saves.\n\nExpression points shown in cyan\ncan be edited. Gray points belong\nto other parts of the mesh.', wraplength=220).pack(pady=18)
        self.status = tk.StringVar()
        ttk.Label(root, textvariable=self.status, padding=8).pack(fill='x')
        self.scale, self.cx, self.cy = 2, 400, 350
        self.canvas.bind('<Configure>', lambda e: self.draw())
        self.canvas.bind('<Button-1>', self.press)
        self.canvas.bind('<B1-Motion>', self.move)
        self.canvas.bind('<ButtonRelease-1>', lambda e: setattr(self, 'drag', None))
        self.canvas.bind('<MouseWheel>', self.zoom)
        self.canvas.bind('<Button-3>', lambda e: setattr(self, 'pan', (e.x, e.y)))
        self.canvas.bind('<B3-Motion>', self.pan_move)
        root.bind('<Control-z>', lambda e: self.undo())
        root.bind('<Control-s>', lambda e: self.save())
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.load(MODELS / 'NukudeFlat.h')
        root.after(100, self.fit)

    def dirty(self):
        return self.model and self.model.arrays != self.model.original

    def discard(self):
        return not self.dirty() or messagebox.askyesno('Unsaved edits', 'Discard unsaved edits?')

    def close(self):
        if self.discard():
            self.root.destroy()

    def open(self):
        if not self.discard():
            return
        path = filedialog.askopenfilename(initialdir=MODELS, filetypes=[('Model headers', '*.h')])
        if path:
            self.load(path)

    def load(self, path):
        try:
            model = Model(path)
        except Exception as exc:
            messagebox.showerror('Cannot open model', str(exc))
            return
        self.model = model
        self.history.clear()
        self.selected.clear()
        self.modes['values'] = ['Base shape'] + list(model.morphs)
        self.mode.set('Base shape')
        self.fit()

    def change_mode(self, event=None):
        self.selected.clear()
        self.weight.set(1)
        self.draw()

    def axes(self):
        return {'XY': (0, 1), 'XZ': (0, 2), 'YZ': (1, 2)}[self.plane.get()]

    def editable(self):
        if self.mode.get() in self.model.morphs:
            return self.model.arrays[self.model.morphs[self.mode.get()][0]]
        return list(range(len(self.model.arrays['basisVertices'])))

    def fit(self):
        if not self.model:
            return
        a, b = self.axes()
        vs = self.model.vertices(self.mode.get())
        lo = [min(v[i] for v in vs) for i in (a, b)]
        hi = [max(v[i] for v in vs) for i in (a, b)]
        w, h = max(500, self.canvas.winfo_width()), max(400, self.canvas.winfo_height())
        self.scale = min((w-90)/max(1, hi[0]-lo[0]), (h-90)/max(1, hi[1]-lo[1]))
        self.cx = w/2 - (lo[0]+hi[0])/2*self.scale
        self.cy = h/2 + (lo[1]+hi[1])/2*self.scale
        self.draw()

    def draw(self):
        if not self.model:
            return
        self.canvas.delete('all')
        a, b = self.axes()
        vs = self.model.vertices(self.mode.get(), self.weight.get())
        self.points = [(self.cx+v[a]*self.scale, self.cy-v[b]*self.scale) for v in vs]
        topology = 'WavyTriangles' if self.mode.get() == 'Wavy' and self.weight.get() > .5 and 'WavyTriangles' in self.model.arrays else 'basisIndexes'
        for tri in self.model.arrays[topology]:
            coords = [c for i in tri for c in self.points[int(i)]]
            self.canvas.create_polygon(coords, fill='#207d78', outline='#3ca49c' if self.wire.get() else '')
        editable = set(self.editable())
        for i, (x, y) in enumerate(self.points):
            color = '#ffbf69' if i in self.selected else '#70eee0' if i in editable else '#65717d'
            r = 5 if i in self.selected else 3
            self.canvas.create_oval(x-r, y-r, x+r, y+r, fill=color, outline='')
            if self.labels.get():
                self.canvas.create_text(x+8, y-8, text=str(i), fill='#d9e8ef')
        self.info.config(text=f'{len(self.selected)} point(s) selected')
        if len(self.selected) == 1:
            v = vs[next(iter(self.selected))]
            for var, value in zip(self.coords, v):
                var.set(f'{value:.4f}')
        self.status.set(f'{self.model.path.name}  |  {len(vs)} vertices  |  {self.mode.get()}  |  Preview {self.weight.get():.0%}' + ('  |  Unsaved edits' if self.dirty() else ''))

    def snapshot(self):
        self.history.append(copy.deepcopy(self.model.arrays))
        self.history = self.history[-100:]

    def press(self, e):
        ids = self.editable()
        i = min(ids, key=lambda i: math.dist(self.points[i], (e.x, e.y)), default=None)
        if i is None or math.dist(self.points[i], (e.x, e.y)) > 14:
            self.selected.clear()
        elif e.state & 1:
            self.selected.symmetric_difference_update({i})
        else:
            if i not in self.selected:
                self.selected = {i}
            if self.mode.get() == 'Base shape' or self.weight.get() > .999:
                self.snapshot()
                self.drag = (e.x, e.y)
        self.draw()

    def shift(self, delta):
        if self.mode.get() in self.model.morphs:
            ids, name = self.model.morphs[self.mode.get()]
            mapping = {v: i for i, v in enumerate(self.model.arrays[ids])}
        else:
            name = 'basisVertices'
            mapping = {i: i for i in self.selected}
        for i in self.selected:
            row = self.model.arrays[name][mapping[i]]
            self.model.arrays[name][mapping[i]] = [x+d for x, d in zip(row, delta)]

    def move(self, e):
        if self.drag is None:
            return
        a, b = self.axes()
        delta = [0., 0., 0.]
        delta[a], delta[b] = (e.x-self.drag[0])/self.scale, -(e.y-self.drag[1])/self.scale
        self.shift(delta)
        self.drag = (e.x, e.y)
        self.draw()

    def set_point(self):
        if len(self.selected) != 1:
            return
        try:
            target = [float(v.get()) for v in self.coords]
            if not all(math.isfinite(x) and abs(x) < 1e6 for x in target):
                raise ValueError('Use finite coordinates smaller than 1,000,000.')
            old = self.model.vertices(self.mode.get())[next(iter(self.selected))]
            self.snapshot()
            self.shift([x-y for x, y in zip(target, old)])
            self.weight.set(1)
            self.draw()
        except ValueError as exc:
            messagebox.showerror('Invalid coordinate', str(exc))

    def zoom(self, e):
        factor = 1.15 if e.delta > 0 else 1/1.15
        if .01 < self.scale*factor < 1000:
            self.cx = e.x + (self.cx-e.x)*factor
            self.cy = e.y + (self.cy-e.y)*factor
            self.scale *= factor
            self.draw()

    def pan_move(self, e):
        self.cx += e.x-self.pan[0]
        self.cy += e.y-self.pan[1]
        self.pan = (e.x, e.y)
        self.draw()

    def undo(self):
        if self.history:
            self.model.arrays = self.history.pop()
            self.draw()

    def save(self):
        try:
            backup = self.model.save()
            self.draw()
            if backup:
                messagebox.showinfo('Saved', f'Saved {self.model.path.name}\nBackup: {backup.name}\n\nRebuild the simulator or firmware to use the new shape.')
        except Exception as exc:
            messagebox.showerror('Cannot save', str(exc))


if __name__ == '__main__':
    app = tk.Tk()
    Editor(app)
    app.mainloop()
