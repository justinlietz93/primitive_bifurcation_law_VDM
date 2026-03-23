# cf000_symbolic_runtime

Minimal stage-correct CF000 witness runtime.

What it does:
- starts from one realized origin witness `w`
- admits first-axis articulation without any carrier, graph, lattice, metric, Laplacian, or QGT
- continues same-axis bifurcation symbolically
- resolves same-axis signature collisions by orthogonal admission into 2D
- writes observables and a final symbolic snapshot

Build:
```bash
make
```

Run:
```bash
./cf000_run 2000 10 runs/demo
```

Outputs:
- `runs/demo_observables.csv`
- `runs/demo_snapshot.csv`
- `runs/demo_summary.txt`

Plot:
```bash
python3 scripts/plot_run.py runs/demo
```
