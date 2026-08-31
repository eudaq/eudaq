from __future__ import annotations

import json
import re

from pathlib import Path


PMT_PATTERN = re.compile(r"^(M\d+)([CS])$")


class ADCMapping:
    def __init__(
        self,
        adc_channels_file: str | Path,
        modules_file: str | Path,
    ) -> None:

        with open(adc_channels_file, encoding="utf-8") as file:
            self._adc_channels: dict[str, str] = json.load(file)

        with open(modules_file, encoding="utf-8") as file:
            self._modules: dict[str, list[int]] = json.load(file)

    def get_channel_name(
        self,
        channel: int,
    ) -> str:

        return self._adc_channels[str(channel)]

    def is_pmt_channel(
        self,
        channel: int,
    ) -> bool:

        name = self.get_channel_name(channel)

        return PMT_PATTERN.match(name) is not None

    def get_pmt_channels_info(
        self,
    ) -> dict[int, dict[str, int | str]]:

        result: dict[int, dict[str, int | str]] = {}

        for channel_str, detector_name in self._adc_channels.items():
            match = PMT_PATTERN.match(detector_name)

            if match is None:
                continue

            module_name, pmt_type = match.groups()

            if module_name not in self._modules:
                raise ValueError(
                    f"Unknown module '{module_name}' for ADC channel {channel_str}"
                )

            module_column, module_row = self._modules[module_name]

            result[int(channel_str)] = {
                "channel": int(channel_str),
                "name": detector_name,
                "module": module_name,
                "type": pmt_type,
                "row": module_row,
                "column": module_column,
            }

        return result

    def get_channels_table(
        self,
    ) -> list[dict[str, int | str | None]]:

        rows: list[dict[str, int | str | None]] = []

        for channel_str, detector_name in self._adc_channels.items():
            row: dict[str, int | str | None] = {
                "channel": int(channel_str),
                "name": detector_name,
                "module": None,
                "type": None,
                "row": None,
                "column": None,
            }

            match = PMT_PATTERN.match(detector_name)

            if match is not None:
                module_name, pmt_type = match.groups()

                if module_name not in self._modules:
                    raise ValueError(
                        f"Unknown module '{module_name}' for ADC channel {channel_str}"
                    )

                module_column, module_row = self._modules[module_name]
                row.update(
                    {
                        "module": module_name,
                        "type": pmt_type,
                        "row": module_row,
                        "column": module_column,
                    }
                )

            rows.append(row)

        return rows
