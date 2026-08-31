"""HiDRA monitor frontend — Dash entry point.

Two ways to launch this:

  * `python app.py`                     — Flask dev server (CLI args, debug, etc.)
  * `gunicorn -w 1 -b :8050 app:server` — production WSGI server (no warning)

The WSGI path imports the module-level `server` object built by
`build_app()`. To override the config path under gunicorn, set the
`HIDRA_FRONTEND_CONFIG` environment variable.
"""

from __future__ import annotations

import argparse
import importlib.util
import logging
import os
import pkgutil
import sys
from pathlib import Path

# Dash currently calls pkgutil.find_loader internally; Python 3.14 removed it.
if not hasattr(pkgutil, "find_loader"):
    def _find_loader_compat(name: str):
        spec = importlib.util.find_spec(name)
        return spec.loader if spec is not None else None

    pkgutil.find_loader = _find_loader_compat

import dash

from hidra_frontend.backend_client import BackendClient
from hidra_frontend.callbacks import register_all
from hidra_frontend.config import load_config
from hidra_frontend.decoders import get_decoder
from hidra_frontend.layout import build, build_panels
from hidra_frontend.overlay import OverlayStore


DEFAULT_CONFIG_PATH = str(Path(__file__).parent / "config.yaml")


def build_app(config_path: str | None = None) -> dash.Dash:
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s - %(message)s")
    log = logging.getLogger("hidra_frontend")

    config = load_config(config_path or DEFAULT_CONFIG_PATH)
    log.info(
        "loaded config: %d tab(s), backend=%s, decoder=%s",
        len(config.tabs), config.backend.url, config.decoder,
    )

    decoder = get_decoder(config.decoder)
    client = BackendClient(config.backend.url, config.backend.request_timeout_s)
    overlay_store = OverlayStore(config.overlay.search_dir)

    available = client.list_histograms()
    if available:
        log.info("backend exposes %d histograms: %s", len(available), ", ".join(available))
    else:
        log.warning("backend at %s is not reachable yet — UI will retry on each poll", config.backend.url)

    panels_by_tab = build_panels(config, available, client)

    app = dash.Dash(__name__, title="HiDRA Monitor", update_title='')
    app.layout = build(config, panels_by_tab)
    register_all(app, config, panels_by_tab, client, overlay_store, decoder)
    return app


# Module-level WSGI entry point used by `gunicorn app:server`. Built only
# when this file is imported (e.g. by gunicorn), not when it's run as a
# script — `main()` builds its own instance from CLI args in that case.
# `HIDRA_FRONTEND_CONFIG` overrides the default config.yaml location.
if __name__ != "__main__":
    app = build_app(os.environ.get("HIDRA_FRONTEND_CONFIG"))
    server = app.server


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="HiDRA monitor frontend (dev server)")
    parser.add_argument("--config", default=DEFAULT_CONFIG_PATH, help="path to config.yaml")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8050)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args(argv)

    dev_app = build_app(args.config)
    log = logging.getLogger("hidra_frontend")
    log.info("dashboard listening on http://%s:%d (Flask dev server)", args.host, args.port)
    run_kwargs = dict(host=args.host, port=args.port, debug=args.debug)
    if args.debug:
        # The Werkzeug reloader (active only with --debug) watches imported
        # Python modules but not config.yaml. Add it to `extra_files` so a
        # change to the declared layout reloads the app too.
        run_kwargs["extra_files"] = [os.path.abspath(args.config)]
    dev_app.run(**run_kwargs)
    return 0


if __name__ == "__main__":
    sys.exit(main())
