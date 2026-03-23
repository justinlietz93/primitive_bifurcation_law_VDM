from pathlib import Path
import sys
import pandas as pd
import matplotlib.pyplot as plt
prefix = Path(sys.argv[1] if len(sys.argv) > 1 else "runs/demo")
df = pd.read_csv(prefix.with_name(prefix.name + "_observables.csv"))
plt.figure(figsize=(10,6))
for col in ["unresolved_terms","mirror_terms","self_attempt_terms","reexpression_terms","articulation_terms"]:
    plt.plot(df["step"], df[col], label=col)
plt.xlabel("step")
plt.legend()
plt.tight_layout()
out = prefix.with_name(prefix.name + "_observables.png")
plt.savefig(out, dpi=150)
print(out)
