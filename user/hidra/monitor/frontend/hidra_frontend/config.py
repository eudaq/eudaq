"""YAML config loader.

Reads `config.yaml`, validates the basic shape, and returns a
typed `Config` object. The rest of the codebase only touches this
typed object, never the raw YAML — so misspelled keys fail loudly
here instead of producing weird behaviour later.

The dataclasses below mirror the sections of `config.yaml`. If you
add a new section to the YAML, mirror it here and load it in
`load_config()`.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml


@dataclass
class BackendCfg:
    url: str
    request_timeout_s: float


@dataclass
class PollingCfg:
    default_ms: int
    floor_ms: int
    choices_ms: list[int]
    server_pump_ms_hint: int


@dataclass
class OverlayCfg:
    enabled: bool
    search_dir: Path
    default_file: str | None
    # How to scale live vs reference for comparison: "area" (each genuine
    # distribution divided by its own sum of counts, so both integrate to 1)
    # or "none" (raw counts). Profiles/per-channel plots always overlay raw.
    normalize: str


@dataclass
class UICfg:
    # Local hour (0-23) at which the daily "cosmic-ray shower" animation
    # fires automatically. None disables the scheduled trigger (the
    # animation is still available on demand via a triple-click on the
    # title).
    shower_hour: int | None = None


@dataclass
class HistogramDisplayCfg:
    """Per-histogram display options, keyed by histogram name in the
    top-level `histogram_options:` section of config.yaml.

    * `logx`    - render the x-axis on a logarithmic scale. Only applied
                  when every bin edge is positive (a log axis cannot show
                  x <= 0); otherwise it falls back to linear.
    * `density` - divide each bin's content by its (linear) bin width, so
                  histograms with non-uniform binning show a comparable
                  height per unit x instead of raw per-bin counts.
    * `board_hover` - for a per-channel histogram (x-axis title "channel"),
                  also show the board number and the channel within the board
                  in the hover. The board size comes from the top-level `fers:`
                  config section, not repeated here. Resolved into
                  `channels_per_board` below (0 = off) for the generic renderer.
    * `show_flow` - draw ROOT's underflow/overflow bins as extra bars at the
                  edges of a 1D bar chart, so out-of-range entries (e.g. the
                  "no trigger mask" events in the underflow) are visible and
                  the bars sum to the title's entry count.
    """

    logx: bool = False
    density: bool = False
    show_flow: bool = False
    # Resolved board size for the per-channel hover (0 = no board hover). Set
    # from the `board_hover` YAML flag using the `fers:` section; kept as a
    # plain int so figure_builder stays detector-agnostic.
    channels_per_board: int = 0


@dataclass
class FersCfg:
    """FERS detector properties (top-level `fers:` section of config.yaml).

    These describe the hardware, not any one plot, so they live here once
    instead of being repeated per panel/histogram. `channels_per_board` is
    fixed by the FERS board (64); it sizes the channel-selector board labels
    and the per-channel `board_hover`.
    """

    channels_per_board: int = 64


@dataclass
class PanelCfg:
    type: str
    params: dict[str, Any]


@dataclass
class TabCfg:
    id: str
    label: str
    panels: list[PanelCfg]


@dataclass
class Config:
    backend: BackendCfg
    polling: PollingCfg
    overlay: OverlayCfg
    tabs: list[TabCfg]
    ui: UICfg = field(default_factory=UICfg)
    decoder: str = "pure"
    config_dir: Path = field(default_factory=Path)
    histogram_options: dict[str, HistogramDisplayCfg] = field(default_factory=dict)
    fers: FersCfg = field(default_factory=FersCfg)


def _normalize_panel(raw: dict[str, Any]) -> PanelCfg:
    panel_type = raw["type"]
    params = {k: v for k, v in raw.items() if k != "type"}

    # channel_selector: expand template+range into explicit names list
    if panel_type == "channel_selector" and "names" not in params:
        if "template" in params and "range" in params:
            lo, hi = params["range"]
            params["names"] = [params["template"].format(ch=i) for i in range(lo, hi + 1)]

    return PanelCfg(type=panel_type, params=params)


def load_config(path: str | Path) -> Config:
    path = Path(path).resolve()
    with path.open() as f:
        raw = yaml.safe_load(f)

    config_dir = path.parent

    backend = BackendCfg(
        url=raw["backend"]["url"],
        request_timeout_s=float(raw["backend"].get("request_timeout_s", 2.0)),
    )

    p = raw["polling"]
    polling = PollingCfg(
        default_ms=int(p["default_ms"]),
        floor_ms=int(p["floor_ms"]),
        choices_ms=[int(x) for x in p["choices_ms"]],
        server_pump_ms_hint=int(p.get("server_pump_ms_hint", 0)),
    )

    o = raw.get("overlay") or {}
    # Overlay reference directory: only a fallback (the running monitor reports
    # the real snapshot dir over HTTP). May be absolute; a relative path is
    # resolved against the config file's directory, not the launch CWD.
    search_dir = Path(o.get("search_dir", "reference"))
    if not search_dir.is_absolute():
        search_dir = (config_dir / search_dir).resolve()
    overlay = OverlayCfg(
        enabled=bool(o.get("enabled", False)),
        search_dir=search_dir,
        default_file=o.get("default_file"),
        normalize=str(o.get("normalize", "none")),
    )

    tabs: list[TabCfg] = []
    for t in raw["tabs"]:
        tabs.append(
            TabCfg(
                id=t["id"],
                label=t["label"],
                panels=[_normalize_panel(p) for p in t["panels"]],
            )
        )

    decoder = str(raw.get("decoder", "pure"))

    fers_raw = raw.get("fers") or {}
    fers_cfg = FersCfg(channels_per_board=int(fers_raw.get("channels_per_board", FersCfg.channels_per_board)))

    histogram_options: dict[str, HistogramDisplayCfg] = {}
    for name, opts in (raw.get("histogram_options") or {}).items():
        opts = opts or {}
        histogram_options[name] = HistogramDisplayCfg(
            logx=bool(opts.get("logx", False)),
            density=bool(opts.get("density", False)),
            channels_per_board=(fers_cfg.channels_per_board if opts.get("board_hover") else 0),
            show_flow=bool(opts.get("show_flow", False)),
        )

    u = raw.get("ui_effects") or {}
    raw_hour = u.get("shower_hour")
    shower_hour = int(raw_hour) if raw_hour is not None else None
    if shower_hour is not None and not 0 <= shower_hour <= 23:
        raise ValueError(f"ui_effects.shower_hour must be in 0..23 (or null), got {shower_hour}")
    ui = UICfg(shower_hour=shower_hour)

    return Config(
        backend=backend,
        polling=polling,
        overlay=overlay,
        tabs=tabs,
        ui=ui,
        decoder=decoder,
        config_dir=config_dir,
        histogram_options=histogram_options,
        fers=fers_cfg,
    )
