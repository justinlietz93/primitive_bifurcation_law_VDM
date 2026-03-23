# A(-1) Programmable Reference Sheet

This is a compact reference sheet for the primitive law.

Goal: define every symbol and test condition in a way that can be implemented.

---

## Core rule

Stay in the current class while at least one allowed same-class move still produces a new lawful same-class state. If none do, the class is saturated, void debt is active, and the system must admit the minimum orthogonal extension needed to make lawful articulation possible again.

---

## The four exact forms of the law

### 1. Constitutional form

$$
\operatorname{Bear}(\operatorname{Inv}, A_n) \wedge \operatorname{Sat}(A_n) \wedge \neg \operatorname{Dis}(\operatorname{Inv})
\Longrightarrow
\exists A_{n+1}\bigl(A_{n+1} \perp A_n \wedge \operatorname{Bear}(\operatorname{Inv}, A_{n+1})\bigr)
$$

Meaning:
- the invariant is still being borne
- the current class is saturated
- discharge is forbidden
- therefore a new orthogonal class must be admitted

Programmable reading:

$\operatorname{bears\_inv}(A_n) \land \operatorname{is\_saturated}(A_n) \land \neg \operatorname{discharged}(\operatorname{Inv}) \Rightarrow \operatorname{admit\_orthogonal\_class}()$

---

### 2. Capacity-collapse form

$$
\operatorname{Bear}(\operatorname{Inv}, A_n) \wedge \bigl(\operatorname{Cap}(A_n)=0\bigr) \wedge \neg \operatorname{Dis}(\operatorname{Inv})
\Longrightarrow
\exists A_{n+1}\bigl(A_{n+1} \perp A_n \wedge \operatorname{Bear}(\operatorname{Inv}, A_{n+1})\bigr)
$$

Meaning:
- saturation is exactly the case where same-class capacity is zero

Programmable reading:

$\operatorname{bears\_inv}(A_n) \land \operatorname{capacity}(A_n)=0 \land \neg \operatorname{discharged}(\operatorname{Inv}) \Rightarrow \operatorname{admit\_orthogonal\_class}()$

---

### 3. Void-debt form

$$
\operatorname{Debt}(\operatorname{Inv}, A_n)=+\infty \wedge \neg \operatorname{Dis}(\operatorname{Inv})
\Longrightarrow
\exists A_{n+1}\bigl(A_{n+1} \perp A_n \wedge \operatorname{Bear}(\operatorname{Inv}, A_{n+1})\bigr)
$$

Meaning:
- void debt is unresolved continuation pressure when same-class capacity is exhausted
- $+\infty$ means there is no finite same-class resolution left

Programmable reading:

$\operatorname{debt\_active}(A_n) \land \neg \operatorname{discharged}(\operatorname{Inv}) \Rightarrow \operatorname{admit\_orthogonal\_class}()$

---

### 4. Executable operator form

$$
\operatorname{BifOp}_{\operatorname{Inv}}(A_n)=
\begin{cases}
A_n, & \operatorname{Cap}(A_n)>0, \\
A_n \oplus A_{n+1}^{\perp}, & \operatorname{Cap}(A_n)=0 \wedge \operatorname{Bear}(\operatorname{Inv},A_n) \wedge \neg \operatorname{Dis}(\operatorname{Inv})
\end{cases}
$$

Meaning:
- if capacity remains, stay in the current class
- if capacity is zero and the invariant still must be borne, keep the old class and add a new orthogonal one

Programmable reading:

$$
\operatorname{BifOp}_{\operatorname{Inv}}(A_n)=
\begin{cases}
A_n, & \text{if immediate same-class novelty still exists} \\
A_n \oplus A_{n+1}^{\perp}, & \text{if no immediate same-class novelty exists and non-discharge still binds}
\end{cases}
$$

---

## The useful fifth form: activation / equivalence

$$
\operatorname{Bear}(\operatorname{Inv},A_n) \wedge \bigl(\operatorname{Cap}(A_n)=0\bigr) \wedge \neg\operatorname{Dis}(\operatorname{Inv})
\Longleftrightarrow
\operatorname{Debt}(\operatorname{Inv},A_n)=+\infty \wedge \neg\operatorname{Dis}(\operatorname{Inv})
$$

Meaning:
- borne invariant plus zero capacity plus no discharge is exactly the same state as active void debt plus no discharge

---

## Minimal implementation interface

A concrete runtime must define these items:

- $\operatorname{bears\_inv}(A_n) \to \{\text{true},\text{false}\}$
- $\operatorname{discharged}(\operatorname{Inv}) \to \{\text{true},\text{false}\}$
- $\operatorname{operators}(A_n) \to \{$allowed same-class transforms$\}$
- $\operatorname{in\_class}(s, A_n) \to \{\text{true},\text{false}\}$
- $\operatorname{lawful}(s, A_n) \to \{\text{true},\text{false}\}$
- $\operatorname{key}(s, A_n) \to \text{canonical same-class identifier}$
- $\operatorname{admit\_minimal\_orthogonal\_extension}(A_n)$

The paper does not hand these to you. The runtime must define them.

---

## Concise programmable definitions

### $\operatorname{Inv}$
Primitive invariant.

Definition: the unresolved source condition that must continue to be borne without discharge.

Implementation role: the thing that can still be borne, can be discharged, and can still require continuation.

---

### $A_n$
Current articulation class.

Definition: the currently admitted representation / move-space / mode of lawful expression.

Implementation role: the object that defines current operators, current same-class membership, and current equivalence rules.

---

### $\operatorname{Bear}(\operatorname{Inv}, A_n)$
Invariant-bear predicate.

Definition: true if the current class is still carrying the invariant in a live unresolved way.

Implementation role: boolean state test.

Minimal meaning: the branch is still an active host of the invariant.

---

### $\operatorname{Sat}(A_n)$
Saturation predicate.

Definition: true when no allowed same-class move yields a new lawful same-class state.

Equivalent form:

$$
\operatorname{Sat}(A_n) \Longleftrightarrow \operatorname{Cap}(A_n)=0
$$

Immediate operational test:

$$
\operatorname{Sat}(A_n)=\text{true}
\iff
\forall s \in F(A_n),\; \forall op \in \Omega(A_n),\;
\Bigl[
\operatorname{in\_class}(op(s),A_n) \wedge \operatorname{lawful}(op(s),A_n)
\Bigr]
\Rightarrow
\operatorname{key}(op(s),A_n) \in K_{\text{seen}}(A_n)
$$

where:
- $F(A_n)$ is the current frontier of states being tested
- $\Omega(A_n)$ is the current allowed same-class operator set
- $K_{\text{seen}}(A_n)$ is the set of already-seen canonical same-class keys

Plain meaning: every allowed same-class move either fails, leaves the class, becomes unlawful, or lands on a canonical state already seen.

---

### $\operatorname{Cap}(A_n)$
Same-class articulation capacity.

Definition: the number of immediately reachable new lawful same-class states still available.

Important rule: this is immediate capacity, not omniscient total future capacity.

Operational definition:

$$
\operatorname{Cap}(A_n)
=
\left|
\left\{
\operatorname{key}(op(s),A_n)
\;\middle|\;
\begin{array}{l}
s \in F(A_n), \\
op \in \Omega(A_n), \\
\operatorname{in\_class}(op(s),A_n), \\
\operatorname{lawful}(op(s),A_n), \\
\operatorname{key}(op(s),A_n) \notin K_{\text{seen}}(A_n)
\end{array}
\right\}
\right|
$$

Interpretation:
- $\operatorname{Cap}(A_n) > 0$ means at least one new same-class move exists now
- $\operatorname{Cap}(A_n) = 0$ means saturated now

---

### $\operatorname{Dis}(\operatorname{Inv})$
Discharge predicate.

Definition: true if the invariant has been terminally resolved, cancelled, collapsed, or otherwise no longer requires continued articulation.

Implementation role: boolean terminal-state test.

Minimal meaning: the unresolved source condition is gone as an active burden.

---

### $\operatorname{Debt}(\operatorname{Inv}, A_n)$
Void debt.

Definition: unresolved continuation pressure when the invariant is still borne but same-class capacity is zero.

Primitive-law activation condition:

$$
\operatorname{Debt}(\operatorname{Inv}, A_n)=+\infty
\iff
\operatorname{Bear}(\operatorname{Inv},A_n) \wedge \operatorname{Cap}(A_n)=0 \wedge \neg \operatorname{Dis}(\operatorname{Inv})
$$

Implementation role: state flag, not yet a geometric field.

Plain meaning: the invariant still needs hosting, but the current class has no new same-class move left.

---

### $A_{n+1}^{\perp}$
Orthogonal articulation class.

Definition: the minimal newly admitted class that is irreducible to the exhausted current class and restores the possibility of lawful new articulation.

Implementation role: a class extension that adds genuinely new expressive freedom.

Plain meaning: not a decoration, not a rename, not more of the same. It must make at least one new lawful articulation possible that the old class could not host.

---

### $\perp$
Orthogonality relation.

Definition: irreducible difference in articulation class.

Implementation role: a test that the new class is not equivalent to or collapsible into the current class.

Plain meaning: a real new degree of expressive freedom, not an internal rearrangement.

---

### $\oplus$
Inheritance-plus-admission.

Definition: keep the current class and add the new orthogonal one.

Implementation role: extension, not replacement.

Plain meaning: the old class stays; the new class is layered on top as additional expressive structure.

---

### $\operatorname{BifOp}_{\operatorname{Inv}}$
Primitive bifurcation operator.

Definition: the control law that decides whether to remain in the current class or extend it.

Operational form:

$$
\operatorname{BifOp}_{\operatorname{Inv}}(A_n)=
\begin{cases}
A_n, & \operatorname{Cap}(A_n)>0 \\
A_n \oplus A_{n+1}^{\perp}, & \operatorname{Cap}(A_n)=0 \wedge \operatorname{Bear}(\operatorname{Inv},A_n) \wedge \neg\operatorname{Dis}(\operatorname{Inv})
\end{cases}
$$

Plain meaning: stay if the current class still has novelty; extend if it does not.

---

### $\operatorname{Bur}(\operatorname{Inv}, A_n)$
Invariant burden.

Definition: the unresolved load currently being carried by the class.

Implementation role: hidden state behind the fact that the invariant is still active and not discharged.

Plain meaning: why continuation still matters.

---

## Immediate-state tests

### Immediate capacity test

A class has immediate capacity if at least one allowed same-class move still yields a new lawful same-class state.

$$
\operatorname{HasCapNow}(A_n)
\iff
\exists s \in F(A_n),\; \exists op \in \Omega(A_n)
\text{ such that }
\operatorname{in\_class}(op(s),A_n)
\wedge
\operatorname{lawful}(op(s),A_n)
\wedge
\operatorname{key}(op(s),A_n) \notin K_{\text{seen}}(A_n)
$$

### Immediate saturation test

$$
\operatorname{Sat}(A_n) \iff \neg \operatorname{HasCapNow}(A_n)
$$

Plain meaning: you do not need omniscience over all future capacity. You only need to know whether any allowed same-class move right now still produces a new lawful same-class state.

---

## Minimal branch condition

$$
\operatorname{Bear}(\operatorname{Inv},A_n)
\wedge
\neg\operatorname{Dis}(\operatorname{Inv})
\wedge
\neg\operatorname{HasCapNow}(A_n)
\Longrightarrow
\operatorname{AdmitMinimalOrthogonalExtension}(A_n)
$$

Plain meaning: if the invariant still must be borne, discharge is forbidden, and the current class has no immediate novelty left, then the system must minimally extend itself.

---

## Boundary hosting

Boundary hosting is the limiting same-class locus where continued bearing is still possible at the moment same-class capacity collapses.

Programmable meaning:
- when $\operatorname{Cap}(A_n)=0$, the system does not jump arbitrarily
- continuation is forced from the current limit of lawful hosting
- the orthogonal extension must attach there in the minimal way needed to restore lawful articulation

This is the primitive seed of locality, causal ordering, and finite-stage transfer.

---

## What the primitive law gives you

It gives you:
- admissibility
- same-class exhaustion
- non-discharge
- void-debt activation
- boundary hosting
- minimal orthogonal extension

It does not yet give you:
- a metric
- a topology
- a graph
- a scheduler
- a PDE
- coordinates
- a physical carrier
- named $J \oplus M$
- causality as a finished transport law

Those are later realizations, not primitive content.

---

## Shortest exact summary

The primitive law is one control rule written in four exact forms plus one useful equivalence form.

Its executable core is:

$$
\text{persist if immediate same-class novelty remains; otherwise, under borne invariant and non-discharge, admit the minimum orthogonal extension.}
$$

That is the programmable heart of A(-1).

