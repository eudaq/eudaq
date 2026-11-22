"""Shared colors and Plotly layout dict."""

import plotly.graph_objects as go

# Font family shared by the UI (CSS) and the Plotly figures, so plot text
# speaks the same visual language as the rest of the dashboard.
FONT_FAMILY = "monospace"

BG = "#1e1e2e"
BG_ALT = "#181825"
FG = "#cdd6f4"
ACCENT = "#cba6f7"
OK = "#a6e3a1"
WARN = "#f9e2af"
ERR = "#f38ba8"
PRIMARY = "#89b4fa"
SECONDARY = "#a6e3a1"
REFERENCE = "#f9e2af"
BORDER = "#45475a"
SURFACE = "#313244"
# Flat fill for a mapped detector cell that exists but has no data this poll
# (e.g. a PMT channel whose ADC mean was never filled) — visibly distinct from
# both the coloured value cells and the blank (unmapped) background.
EMPTY = "#585b70"
GRID = "rgba(205, 214, 244, 0.10)"
ZERO = "rgba(205, 214, 244, 0.16)"

# Catppuccin Mocha accent colours, ordered for good separation when several
# series are overlaid on the dark background. Cycle through it for per-series
# line colours (e.g. one per tracker station).
PALETTE = (
    PRIMARY,    # blue    #89b4fa
    "#fab387",  # peach
    OK,         # green   #a6e3a1
    ACCENT,     # mauve   #cba6f7
    WARN,       # yellow  #f9e2af
    "#94e2d5",  # teal
    "#f5c2e7",  # pink
    ERR,        # red     #f38ba8
)


def base_figure_layout(title: str) -> dict:
    # uirevision keyed on the figure title (= histogram name) preserves
    # user zoom/pan/legend state across data refreshes. Plotly resets UI
    # state only when uirevision changes.
    return dict(
        title=title,
        margin=dict(l=40, r=20, t=40, b=40),
        paper_bgcolor=BG,
        plot_bgcolor=BG,
        font=dict(color=FG, family=FONT_FAMILY),
        xaxis=dict(
            showgrid=True,
            gridcolor=GRID,
            gridwidth=1,
            zeroline=True,
            zerolinecolor=ZERO,
            zerolinewidth=1,
        ),
        yaxis=dict(
            showgrid=True,
            gridcolor=GRID,
            gridwidth=1,
            zeroline=True,
            zerolinecolor=ZERO,
            zerolinewidth=1,
        ),
        uirevision=title,
    )


def placeholder_figure(title: str) -> go.Figure:
    """Empty figure pre-styled with the dashboard theme.

    Used as initial `dcc.Graph` figure so that, before the first poll
    delivers data, the user doesn't see Plotly's default white grid.
    """
    fig = go.Figure()
    fig.update_layout(**base_figure_layout(title))
    return fig
