"""Lightweight phase timing for the polling pipeline.

Two pieces:

  * `Phase("name")` — a context manager. Wrap a block of code:

        with Phase("fetch_multi"):
            ...

    Nested `Phase` blocks build a parent/child hierarchy automatically
    (via a thread-local stack), so the timings show up as a tree in
    the summary. No need to spell out the hierarchy in the names.

  * `PERF.summary_lines()` — returns the current accumulated timings
    formatted as a tree. The poll callback prints this periodically
    to the log.

The overhead per Phase is in the microsecond range (one perf_counter
call + a dict update under a lock), so it's fine to leave timers on
in production.
"""

from __future__ import annotations

import threading
import time
from collections import defaultdict
from threading import Lock


# Each thread keeps its own stack of currently-open Phase names. This
# is how we know, when a Phase exits, what its parent was. Dash uses
# Werkzeug's threaded server, so concurrent polls don't interfere.
_current_path = threading.local()


def _stack() -> list[str]:
    s = getattr(_current_path, "stack", None)
    if s is None:
        s = []
        _current_path.stack = s
    return s


# Separator used inside accumulator keys to join parent and child
# names. Anything that's unlikely to appear in a Phase name will do.
_SEP = " > "


class PhaseAccumulator:
    """Total time + count, keyed by the fully-qualified path of nested Phases."""

    def __init__(self) -> None:
        # _stats["a > b > c"] = [total_seconds, count]
        self._stats: dict[str, list[float]] = defaultdict(lambda: [0.0, 0])
        self._lock = Lock()

    def add(self, qualified_name: str, dt_seconds: float) -> None:
        with self._lock:
            slot = self._stats[qualified_name]
            slot[0] += dt_seconds
            slot[1] += 1

    def reset(self) -> None:
        with self._lock:
            self._stats.clear()

    def snapshot(self) -> dict[str, tuple[float, int]]:
        with self._lock:
            return {k: (v[0], v[1]) for k, v in self._stats.items()}

    def summary_lines(self) -> list[str]:
        """Format the current accumulator as a tree, one line per node.

        Children are indented under their parent with Unicode tree
        characters; each child also shows its percentage of the
        parent's total time.
        """
        snap = self.snapshot()
        if not snap:
            return ["  (no samples)"]

        # Parse "a > b > c" -> ("a", "b", "c") so we can talk about
        # parents and children as tuples.
        nodes: dict[tuple[str, ...], tuple[float, int]] = {
            tuple(k.split(_SEP)): v for k, v in snap.items()
        }

        # Index children by parent path for fast lookup.
        children: dict[tuple[str, ...], list[tuple[str, ...]]] = defaultdict(list)
        for path in nodes:
            if len(path) > 1:
                children[path[:-1]].append(path)

        lines: list[str] = []

        def render_node(
            path: tuple[str, ...],
            prefix: str,
            is_last: bool,
            parent_total_s: float | None,
        ) -> None:
            total_s, count = nodes[path]
            mean_ms = (total_s / count * 1000.0) if count else 0.0
            pct_str = (
                f"  ({total_s / parent_total_s * 100.0:5.1f}%)"
                if parent_total_s
                else ""
            )

            # Tree drawing characters: └─ for the last child of a
            # parent, ├─ for any other. The vertical bar │ keeps the
            # subtree of "non-last" siblings connected.
            name = path[-1]
            if len(path) == 1:
                tree_prefix = ""
            else:
                tree_prefix = prefix + ("└─ " if is_last else "├─ ")

            # Pad the tree+name block to a fixed width so the
            # `total=`/`n=`/`mean=` columns stay aligned regardless
            # of nesting depth.
            label = f"{tree_prefix}{name}"
            lines.append(
                f"  {label:<48s}"
                f"total={total_s * 1000:8.1f} ms  "
                f"n={count:5d}  "
                f"mean={mean_ms:7.3f} ms"
                f"{pct_str}"
            )

            # Prefix to use for this node's children.
            next_prefix = prefix + ("   " if is_last else "│  ") if len(path) > 1 else ""

            kids = sorted(children.get(path, []), key=lambda p: -nodes[p][0])
            for i, child in enumerate(kids):
                render_node(child, next_prefix, i == len(kids) - 1, total_s)

        # Roots are paths of length 1 (= Phase blocks opened with no
        # outer Phase). Print them sorted by total time descending.
        roots = sorted([p for p in nodes if len(p) == 1], key=lambda p: -nodes[p][0])
        for root in roots:
            render_node(root, "", True, None)

        return lines


PERF = PhaseAccumulator()


class Phase:
    """Context manager: record elapsed time of a block into PERF.

    Use as `with Phase("name"): ...`. When Phase blocks are nested,
    each accumulates its time under its full path (parent > child),
    so the summary can render the call tree.
    """

    __slots__ = ("name", "_t0")

    def __init__(self, name: str) -> None:
        self.name = name

    def __enter__(self) -> "Phase":
        _stack().append(self.name)
        self._t0 = time.perf_counter()
        return self

    def __exit__(self, *_exc) -> None:
        dt = time.perf_counter() - self._t0
        stack = _stack()
        PERF.add(_SEP.join(stack), dt)
        stack.pop()
