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
\mathop{Bear}(\mathop{Inv}, A_n) \wedge \mathop{Sat}(A_n) \wedge \neg \mathop{Dis}(\mathop{Inv})
\Longrightarrow
\exists A_{n+1}\bigl(A_{n+1} \perp A_n \wedge \mathop{Bear}(\mathop{Inv}, A_{n+1})\bigr)
$$

Meaning:

* the invariant is still being borne
* the current class is saturated
* discharge is forbidden
* therefore a new orthogonal class must be admitted

Programmable reading:

$\mathop{bears_inv}(A_n) \land \mathop{is_saturated}(A_n) \land \neg \mathop{discharged}(\mathop{Inv}) \Rightarrow \mathop{admit_orthogonal_class}()$

---

### 2. Capacity-collapse form

$$
\mathop{Bear}(\mathop{Inv}, A_n) \wedge \bigl(\mathop{Cap}(A_n)=0\bigr) \wedge \neg \mathop{Dis}(\mathop{Inv})
\Longrightarrow
\exists A_{n+1}\bigl(A_{n+1} \perp A_n \wedge \mathop{Bear}(\mathop{Inv}, A_{n+1})\bigr)
$$

Meaning:

* saturation is exactly the case where same-class capacity is zero

Programmable reading:

$\mathop{bears_inv}(A_n) \land \mathop{capacity}(A_n)=0 \land \neg \mathop{discharged}(\mathop{Inv}) \Rightarrow \mathop{admit_orthogonal_class}()$

---

### 3. Void-debt form

$$
\mathop{Debt}(\mathop{Inv}, A_n)=+\infty \wedge \neg \mathop{Dis}(\mathop{Inv})
\Longrightarrow
\exists A_{n+1}\bigl(A_{n+1} \perp A_n \wedge \mathop{Bear}(\mathop{Inv}, A_{n+1})\bigr)
$$

Meaning:

* void debt is unresolved continuation pressure when same-class capacity is exhausted
* $+\infty$ means there is no finite same-class resolution left

Programmable reading:

$\mathop{debt_active}(A_n) \land \neg \mathop{discharged}(\mathop{Inv}) \Rightarrow \mathop{admit_orthogonal_class}()$

---

### 4. Executable operator form

$$
\mathop{BifOp}*{\mathop{Inv}}(A_n)=
\begin{cases}
A_n, & \mathop{Cap}(A_n)>0, \
A_n \oplus A*{n+1}^{\perp}, & \mathop{Cap}(A_n)=0 \wedge \mathop{Bear}(\mathop{Inv},A_n) \wedge \neg \mathop{Dis}(\mathop{Inv})
\end{cases}
$$

Meaning:

* if capacity remains, stay in the current class
* if capacity is zero and the invariant still must be borne, keep the old class and add a new orthogonal one

Programmable reading:

$$
\mathop{BifOp}*{\mathop{Inv}}(A_n)=
\begin{cases}
A_n, & \text{if immediate same-class novelty still exists} \
A_n \oplus A*{n+1}^{\perp}, & \text{if no immediate same-class novelty exists and non-discharge still binds}
\end{cases}
$$

---

## The useful fifth form: activation / equivalence

$$
\mathop{Bear}(\mathop{Inv},A_n) \wedge \bigl(\mathop{Cap}(A_n)=0\bigr) \wedge \neg\mathop{Dis}(\mathop{Inv})
\Longleftrightarrow
\mathop{Debt}(\mathop{Inv},A_n)=+\infty \wedge \neg\mathop{Dis}(\mathop{Inv})
$$

Meaning:

* borne invariant plus zero capacity plus no discharge is exactly the same state as active void debt plus no discharge

---

## Minimal implementation interface

A concrete runtime must define these items:

* $\mathop{bears_inv}(A_n) \to {\text{true},\text{false}}$
* $\mathop{discharged}(\mathop{Inv}) \to {\text{true},\text{false}}$
* $\mathop{operators}(A_n) \to {$allowed same-class transforms$}$
* $\mathop{in_class}(s, A_n) \to {\text{true},\text{false}}$
* $\mathop{lawful}(s, A_n) \to {\text{true},\text{false}}$
* $\mathop{key}(s, A_n) \to \text{canonical same-class identifier}$
* $\mathop{admit_minimal_orthogonal_extension}(A_n)$

The paper does not hand these to you. The runtime must define them.

---

## Concise programmable definitions

### $\mathop{Inv}$

Primitive invariant.

Definition: the unresolved source condition that must continue to be borne without discharge.

Implementation role: the thing that can still be borne, can be discharged, and can still require continuation.

---

### $A_n$

Current articulation class.

Definition: the currently admitted representation / move-space / mode of lawful expression.

Implementation role: the object that defines current operators, current same-class membership, and current equivalence rules.

---

### $\mathop{Bear}(\mathop{Inv}, A_n)$

Invariant-bear predicate.

Definition: true if the current class is still carrying the invariant in a live unresolved way.

Implementation role: boolean state test.

Minimal meaning: the branch is still an active host of the invariant.

---

### $\mathop{Sat}(A_n)$

Saturation predicate.

Definition: true when no allowed same-class move yields a new lawful same-class state.

Equivalent form:

$$
\mathop{Sat}(A_n) \Longleftrightarrow \mathop{Cap}(A_n)=0
$$

Immediate operational test:

$$
\mathop{Sat}(A_n)=\text{true}
\iff
\forall s \in F(A_n),; \forall op \in \Omega(A_n),;
\Bigl[
\mathop{in_class}(op(s),A_n) \wedge \mathop{lawful}(op(s),A_n)
\Bigr]
\Rightarrow
\mathop{key}(op(s),A_n) \in K_{\text{seen}}(A_n)
$$

where:

* $F(A_n)$ is the current frontier of states being tested
* $\Omega(A_n)$ is the current allowed same-class operator set
* $K_{\text{seen}}(A_n)$ is the set of already-seen canonical same-class keys

Plain meaning: every allowed same-class move either fails, leaves the class, becomes unlawful, or lands on a canonical state already seen.

---

### $\mathop{Cap}(A_n)$

Same-class articulation capacity.

Definition: the number of immediately reachable new lawful same-class states still available.

Important rule: this is immediate capacity, not omniscient total future capacity.

Operational definition:

$$
\mathop{Cap}(A_n)
=======================

\left|
\left{
\mathop{key}(op(s),A_n)
;\middle|;
\begin{array}{l}
s \in F(A_n), \
op \in \Omega(A_n), \
\mathop{in_class}(op(s),A_n), \
\mathop{lawful}(op(s),A_n), \
\mathop{key}(op(s),A_n) \notin K_{\text{seen}}(A_n)
\end{array}
\right}
\right|
$$

Interpretation:

* $\mathop{Cap}(A_n) > 0$ means at least one new same-class move exists now
* $\mathop{Cap}(A_n) = 0$ means saturated now

---

### $\mathop{Dis}(\mathop{Inv})$

Discharge predicate.

Definition: true if the invariant has been terminally resolved, cancelled, collapsed, or otherwise no longer requires continued articulation.

Implementation role: boolean terminal-state test.

Minimal meaning: the unresolved source condition is gone as an active burden.

---

### $\mathop{Debt}(\mathop{Inv}, A_n)$

Void debt.

Definition: unresolved continuation pressure when the invariant is still borne but same-class capacity is zero.

Primitive-law activation condition:

$$
\mathop{Debt}(\mathop{Inv}, A_n)=+\infty
\iff
\mathop{Bear}(\mathop{Inv},A_n) \wedge \mathop{Cap}(A_n)=0 \wedge \neg \mathop{Dis}(\mathop{Inv})
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

### $\mathop{BifOp}_{\mathop{Inv}}$

Primitive bifurcation operator.

Definition: the control law that decides whether to remain in the current class or extend it.

Operational form:

$$
\mathop{BifOp}*{\mathop{Inv}}(A_n)=
\begin{cases}
A_n, & \mathop{Cap}(A_n)>0 \
A_n \oplus A*{n+1}^{\perp}, & \mathop{Cap}(A_n)=0 \wedge \mathop{Bear}(\mathop{Inv},A_n) \wedge \neg\mathop{Dis}(\mathop{Inv})
\end{cases}
$$

Plain meaning: stay if the current class still has novelty; extend if it does not.

---

### $\mathop{Bur}(\mathop{Inv}, A_n)$

Invariant burden.

Definition: the unresolved load currently being carried by the class.

Implementation role: hidden state behind the fact that the invariant is still active and not discharged.

Plain meaning: why continuation still matters.

---

## Immediate-state tests

### Immediate capacity test

A class has immediate capacity if at least one allowed same-class move still yields a new lawful same-class state.

$$
\mathop{HasCapNow}(A_n)
\iff
\exists s \in F(A_n),; \exists op \in \Omega(A_n)
\text{ such that }
\mathop{in_class}(op(s),A_n)
\wedge
\mathop{lawful}(op(s),A_n)
\wedge
\mathop{key}(op(s),A_n) \notin K_{\text{seen}}(A_n)
$$

### Immediate saturation test

$$
\mathop{Sat}(A_n) \iff \neg \mathop{HasCapNow}(A_n)
$$

Plain meaning: you do not need omniscience over all future capacity. You only need to know whether any allowed same-class move right now still produces a new lawful same-class state.

---

## Minimal branch condition

$$
\mathop{Bear}(\mathop{Inv},A_n)
\wedge
\neg\mathop{Dis}(\mathop{Inv})
\wedge
\neg\mathop{HasCapNow}(A_n)
\Longrightarrow
\mathop{AdmitMinimalOrthogonalExtension}(A_n)
$$

Plain meaning: if the invariant still must be borne, discharge is forbidden, and the current class has no immediate novelty left, then the system must minimally extend itself.

---

## Boundary hosting

Boundary hosting is the limiting same-class locus where continued bearing is still possible at the moment same-class capacity collapses.

Programmable meaning:

* when $\mathop{Cap}(A_n)=0$, the system does not jump arbitrarily
* continuation is forced from the current limit of lawful hosting
* the orthogonal extension must attach there in the minimal way needed to restore lawful articulation

This is the primitive seed of locality, causal ordering, and finite-stage transfer.

---

## Scope note

This sheet is a working computational reference for the primitive law.

It is not a prohibition list.
It is not claiming that later structures are impossible.
It is only naming the control logic that is explicit here so the runtime can be built without semantic drift.

Use it as a stable reference for the trigger, state tests, and branch condition.
Do not read it as a fence around future derivations or implementations.

---

## Shortest exact summary

The primitive law is one control rule written in four exact forms plus one useful equivalence form.

Its executable core is:

$$
\text{persist if immediate same-class novelty remains; otherwise, under borne invariant and non-discharge, admit the minimum orthogonal extension.}
$$

That is the programmable heart of A(-1).
