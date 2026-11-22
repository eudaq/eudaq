"""Calorimeter channel mapping.

Maps ADC channel indices to detector positions (module, row, column,
S/C type). The raw maps live in the bundled JSON files
(`adc_channels.json` = channel -> detector name, `modules.json` =
module -> [column, row]); `ADCMapping` joins them.


Most callers just want "channel index -> info" for the PMT channels —
use the module-level `get_pmt_channel_info()` helper, which loads the
bundled JSON once and caches it.
"""

from __future__ import annotations

from pathlib import Path

from .calo_mapping import ADCMapping
from .maxicc_mapping import MAXICCMapping
from .sipm_mapping import SiPMMapping

_MAPPING_DIR = Path(__file__).parent
_ADC_CHANNELS_FILE = _MAPPING_DIR / "adc_channels.json"
_MODULES_FILE = _MAPPING_DIR / "modules.json"
_SIPM_CHANNELS_FILE = _MAPPING_DIR / "sipm_channels.json"
_MAXICC_CHANNELS_FILE = _MAPPING_DIR / "maxicc_channels.json"

_default_mapping: ADCMapping | None = None
_default_sipm_mapping: SiPMMapping | None = None
_default_maxicc_mapping: MAXICCMapping | None = None


def default_mapping() -> ADCMapping:
    """Return the process-wide `ADCMapping` built from the bundled JSON."""
    global _default_mapping
    if _default_mapping is None:
        _default_mapping = ADCMapping(_ADC_CHANNELS_FILE, _MODULES_FILE)
    return _default_mapping


def get_pmt_channel_info() -> dict[int, dict[str, int | str]]:
    """Channel index -> {channel, name, module, type, row, column}.

    Only PMT channels (names matching `M<n>S` / `M<n>C`) are included;
    non-PMT channels like the muon counter are skipped.
    """
    return default_mapping().get_pmt_channels_info()


def default_sipm_mapping() -> SiPMMapping:
    """Return the process-wide `SiPMMapping` built from the bundled JSON."""
    global _default_sipm_mapping
    if _default_sipm_mapping is None:
        _default_sipm_mapping = SiPMMapping(_SIPM_CHANNELS_FILE)
    return _default_sipm_mapping


def get_sipm_channel_info() -> dict[int, dict[str, int | str]]:
    """FERS channel index -> {channel, column, row, type, module}."""
    return default_sipm_mapping().get_sipm_channels_info()


def default_maxicc_mapping() -> MAXICCMapping:
    """Return the process-wide `MAXICCMapping` built from the bundled JSON."""
    global _default_maxicc_mapping
    if _default_maxicc_mapping is None:
        _default_maxicc_mapping = MAXICCMapping(_MAXICC_CHANNELS_FILE)
    return _default_maxicc_mapping


def get_maxicc_channel_info() -> list[dict[str, int]]:
    """List of MAXICC channels: {board, channel, layer, col, row} (local board)."""
    return default_maxicc_mapping().get_channels_info()


__all__ = [
    "ADCMapping",
    "SiPMMapping",
    "MAXICCMapping",
    "default_mapping",
    "default_sipm_mapping",
    "default_maxicc_mapping",
    "get_pmt_channel_info",
    "get_sipm_channel_info",
    "get_maxicc_channel_info",
]
