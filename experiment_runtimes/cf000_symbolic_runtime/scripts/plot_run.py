from pathlib import Path
import sys
import pandas as pd
import matplotlib.pyplot as plt

prefix = Path(sys.argv[1] if len(sys.argv) > 1 else "runs/demo")
obs = pd.read_csv(prefix.with_name(prefix.name + "_observables.csv"))

plt.figure(figsize=(10, 6))
plt.plot(obs["step"], obs["axis1_terms"], label="axis1_terms")
plt.plot(obs["step"], obs["axis2_terms"], label="axis2_terms")
plt.plot(obs["step"], obs["frontier_terms"], label="frontier_terms")
plt.plot(obs["step"], obs["collisions_resolved"], label="collisions_resolved")
plt.xlabel("step")
plt.legend()
plt.tight_layout()
out = prefix.with_name(prefix.name + "_observables.png")
plt.savefig(out, dpi=150)
print(out)
