PROJECT = dpsweld
BUILD_DIR = bin

# SHARED_DIR = ../my-common-code
CFILES = main.c
CFILES += spi_driver.c
CFILES += ili9163c.c
CFILES += tick.c
CFILES += hw.c
CFILES += ringbuf.c
CFILES += event.c
CFILES += font-meter_small.c
CFILES += font-meter_medium.c
CFILES += font-meter_large.c
CFILES += gfx_lookup.c
CFILES += tft.c
CFILES += pwrctl.c
CFILES += mini-printf.c
# AFILES += api-asm.S

# TODO - you will need to edit these two lines!
DEVICE=stm32f100c8
OOCD_INTERFACE = stlink
OOCD_TARGET = stm32f1x_fix

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk