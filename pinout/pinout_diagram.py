###########################################
#
# Example script to build a
# pinout diagram. Includes basic
# features and convenience classes.
#
###########################################

from pinout.core import Group, Image
from pinout.components.layout import Diagram_2Rows
from pinout.components.pinlabel import PinLabelGroup, PinLabel
from pinout.components.text import TextBlock
from pinout.components import leaderline as lline
from pinout.components.legend import Legend


# Import data for the diagram
import data

# Create a new diagram
# The Diagram_2Rows class provides 2 panels,
# 'panel_01' and 'panel_02', to insert components into.
diagram = Diagram_2Rows(1224, 1320, 1184, "diagram")

# Add a stylesheet
diagram.add_stylesheet("styles.css", embed=True)

# Create a group to hold the pinout-diagram components.
graphic = diagram.panel_01.add(Group(400, 42))

# Add and embed an image
hardware = graphic.add(Image("esp32c3_fpga_crop.png", embed=True))

# Measure and record key locations with the hardware Image instance
hardware.add_coord("j3", 29, 251)
hardware.add_coord("pmod1", 406, 196)
hardware.add_coord("pmod2", 406, 506)
hardware.add_coord("pmod3", 406, 814)
# Other (x,y) pairs can also be stored here
hardware.add_coord("pin_pitch_v", 0, 34)

# Create pinlabels on the left header
graphic.add(
    PinLabelGroup(
        x=hardware.coord("j3").x,
        y=hardware.coord("j3").y,
        pin_pitch=hardware.coord("pin_pitch_v", raw=True),
        label_start=(30, 0),
        label_pitch=(0, 34),
        scale=(-1, 1),
        labels=data.j3,
    )
)

# Create pinlabels on the right headers
graphic.add(
    PinLabelGroup(
        x=hardware.coord("pmod1").x,
        y=hardware.coord("pmod1").y,
        pin_pitch=hardware.coord("pin_pitch_v", raw=True),
        label_start=(30, 0),
        label_pitch=(0, 34),
        labels=data.pmod1,
    )
)

graphic.add(
    PinLabelGroup(
        x=hardware.coord("pmod2").x,
        y=hardware.coord("pmod2").y,
        pin_pitch=hardware.coord("pin_pitch_v", raw=True),
        label_start=(30, 0),
        label_pitch=(0, 34),
        labels=data.pmod2,
    )
)

graphic.add(
    PinLabelGroup(
        x=hardware.coord("pmod3").x,
        y=hardware.coord("pmod3").y,
        pin_pitch=hardware.coord("pin_pitch_v", raw=True),
        label_start=(30, 0),
        label_pitch=(0, 34),
        labels=data.pmod3,
    )
)

# Create a title and description text-blocks
title_block = diagram.panel_02.add(
    TextBlock(
        data.title,
        x=20,
        y=30,
        line_height=18,
        tag="panel title_block",
    )
)
diagram.panel_02.add(
    TextBlock(
        data.description,
        x=20,
        y=60,
        width=title_block.width,
        height=diagram.panel_02.height - title_block.height,
        line_height=18,
        tag="panel text_block",
    )
)

# Create a legend
legend = diagram.panel_02.add(
    Legend(
        data.legend,
        x=340,
        y=8,
        max_height=132,
    )
)

# Export the diagram via commandline:
# >>> py -m pinout.manager --export pinout_diagram.py diagram.svg
