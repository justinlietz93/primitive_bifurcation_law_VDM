"""
A(-1) / Primitive Bifurcation Law
Single-law symbolic runtime.

No lattice.
No coordinates.
No numeric ontology doing theorem work.
No separate domain engines.

This file implements the law as one state transition whose realized face is
selected only by the constitutional failure mode already stated in canon:

Type I   : completion
Type II  : orthogonal re-articulation
Type III : metric / dissipative handling

The state is symbolic and inheritance-preserving.
"""

from __future__ import annotations

from dataclasses import dataclass, field, replace
from typing import Any, Mapping, Tuple
import hashlib


def _stable_token(*parts: Any) -> str:
    """Deterministic symbolic token with no theorem-bearing numeric content."""
    h = hashlib.sha256(repr(parts).encode("utf-8")).hexdigest()
    return h[:16]


@dataclass(frozen=True)
class AMinusOneState:
    inv: Any
    current_class: Any
    borne: bool
    discharged: bool
    artcap_exhausted: bool
    missing_closure: bool = False
    finite_hosting_loss: bool = False
    debt_active: bool = False

    inherited_stack: Tuple[Any, ...] = field(default_factory=tuple)
    channels: Tuple[Any, ...] = field(default_factory=tuple)
    payload: Mapping[str, Any] = field(default_factory=dict)


def _inherit(s: AMinusOneState) -> Tuple[Any, ...]:
    if s.current_class in s.inherited_stack:
        return s.inherited_stack
    return s.inherited_stack + (s.current_class,)


def _completion_marker(s: AMinusOneState) -> Any:
    return ("completion", _stable_token("completion", s.current_class, s.inv))


def _orthogonal_class(s: AMinusOneState) -> Any:
    return ("orthogonal", _stable_token("orthogonal", s.current_class, s.inv))


def _metric_channel(s: AMinusOneState) -> Any:
    return ("metric", _stable_token("metric", s.current_class, s.inv))


def _type_i_completion(s: AMinusOneState) -> AMinusOneState:
    inherited = _inherit(s)
    completion = _completion_marker(s)
    return replace(
        s,
        inherited_stack=inherited,
        channels=s.channels + (completion,),
        missing_closure=False,
        debt_active=False,
    )


def _type_ii_orthogonal_rearticulation(s: AMinusOneState) -> AMinusOneState:
    inherited = _inherit(s)
    new_class = _orthogonal_class(s)
    return replace(
        s,
        current_class=new_class,
        inherited_stack=inherited,
        artcap_exhausted=False,
        debt_active=False,
    )


def _type_iii_metric_handling(s: AMinusOneState) -> AMinusOneState:
    inherited = _inherit(s)
    metric = _metric_channel(s)
    return replace(
        s,
        inherited_stack=inherited,
        channels=s.channels + (metric,),
        finite_hosting_loss=False,
        debt_active=False,
    )


def A_minus_one(s: AMinusOneState) -> AMinusOneState:
    """
    One primitive law, three recurring constitutional resolution modes.

    Claim-form rendering:
        borne ∧ sat ∧ ¬discharged        -> admit orthogonal class
        borne ∧ (artcap == 0) ∧ ¬dis    -> admit orthogonal class
        debt_active ∧ ¬discharged       -> admit orthogonal class

    Mode selection:
        Type I   : missing closure                           -> completion
        Type II  : same-class capacity exhausted, borne,
                   discharge forbidden                       -> orthogonal re-articulation
        Type III : finite hosting approaches operational
                   loss, discharge forbidden                 -> metric / dissipative handling

    If none of the constitutional forcing conditions are active, the class persists.
    """
    if s.missing_closure and s.borne and not s.discharged:
        return _type_i_completion(s)

    if s.artcap_exhausted and s.borne and not s.discharged:
        return _type_ii_orthogonal_rearticulation(s)

    if s.finite_hosting_loss and s.borne and not s.discharged:
        return _type_iii_metric_handling(s)

    return s


if __name__ == "__main__":
    # Minimal symbolic demonstration only.
    state = AMinusOneState(
        inv="Inv",
        current_class=("class", "A_n"),
        borne=True,
        discharged=False,
        artcap_exhausted=True,
        debt_active=True,
    )

    next_state = A_minus_one(state)
    print("before =", state)
    print("after  =", next_state)
