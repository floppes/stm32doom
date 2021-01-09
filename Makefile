TARGET   = stm32doom
LDSCRIPT = memory.ld
SRCDIR   = src
LIBDIR   = lib
BINDIR   = bin
DOOMDIR  = chocdoom

SRC_MAIN = button.c debug.c font.c gfx.c i2c.c images.c jpeg.c lcd.c led.c main.c sdram.c spi.c syscalls.c touch.c vectors.c

SRC_DOOM = dummy.c am_map.c doomdef.c doomstat.c dstrings.c d_event.c d_items.c d_iwad.c d_loop.c d_main.c d_mode.c d_net.c f_finale.c f_wipe.c g_game.c hu_lib.c hu_stuff.c info.c i_cdmus.c i_endoom.c i_joystick.c i_main.c i_scale.c i_sound.c i_system.c i_timer.c i_video.c memio.c m_argv.c m_bbox.c m_cheat.c m_config.c m_controls.c m_fixed.c m_menu.c m_misc.c m_random.c p_ceilng.c p_doors.c p_enemy.c p_floor.c p_inter.c p_lights.c p_map.c p_maputl.c p_mobj.c p_plats.c p_pspr.c p_saveg.c p_setup.c p_sight.c p_spec.c p_switch.c p_telept.c p_tick.c p_user.c r_bsp.c r_data.c r_draw.c r_main.c r_plane.c r_segs.c r_sky.c r_things.c sha1.c sounds.c statdump.c st_lib.c st_stuff.c s_sound.c tables.c v_video.c wi_stuff.c w_checksum.c w_file.c w_file_stdc.c w_main.c w_wad.c z_zone.c

LIB_ST   = misc.c stm32f4xx_dma.c stm32f4xx_dma2d.c stm32f4xx_exti.c stm32f4xx_fmc.c stm32f4xx_gpio.c stm32f4xx_i2c.c stm32f4xx_ltdc.c stm32f4xx_rcc.c stm32f4xx_sdio.c stm32f4xx_spi.c stm32f4xx_syscfg.c stm32f4xx_tim.c stm32f4xx_usart.c system_stm32f4xx.c

LIB_USB  = usb_bsp.c usb_core.c usb_hcd.c usb_hcd_int.c usb_msc_host.c usbh_core.c usbh_hcs.c usbh_ioreq.c usbh_msc_bot.c usbh_msc_core.c usbh_msc_scsi.c usbh_stdreq.c usbh_usr.c

LIB_FAT  = diskio.c fatfs.c fatfs_sdcard.c fatfs_usbdisk.c ff.c

LIB      = $(addprefix stm32/,$(LIB_ST)) $(addprefix usb/,$(LIB_USB)) $(addprefix fatfs/,$(LIB_FAT))

SRC      = $(SRC_MAIN) $(addprefix $(DOOMDIR)/,$(SRC_DOOM))

DEFINES  = -DDEBUG_ -DSTM32F4XX -DUSE_STDPERIPH_DRIVER -DSTM32F429_439xx

CC       = arm-none-eabi-gcc
BIN      = arm-none-eabi-objcopy
OBJDUMP  = arm-none-eabi-objdump
SIZE     = arm-none-eabi-size
CFLAGS   = -mthumb -mtune=cortex-m4 -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mlittle-endian -fcommon -Wall $(DEFINES) -g -I $(SRCDIR) -I $(SRCDIR)/$(DOOMDIR) -I $(LIBDIR)/stm32 -I $(LIBDIR)/usb -I $(LIBDIR)/fatfs -O2 -c
LDFLAGS  = -mthumb -mtune=cortex-m4 -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mlittle-endian -lm -nostartfiles -T$(LDSCRIPT) -Wl,-Map=$(BINDIR)/$(TARGET).map

OBJS     = $(addprefix $(SRCDIR)/, $(SRC:.c=.o)) $(addprefix $(LIBDIR)/, $(LIB:.c=.o))
OBJ      = $(subst $(LIBDIR)/,$(BINDIR)/,$(subst $(SRCDIR)/,$(BINDIR)/,$(OBJS)))


DUMMY:=$(shell if ! [ -d $(BINDIR) ]; then mkdir $(BINDIR); fi)
DUMMY:=$(shell if ! [ -d $(BINDIR)/$(DOOMDIR) ]; then mkdir $(BINDIR)/$(DOOMDIR); fi)
DUMMY:=$(shell if ! [ -d $(BINDIR)/stm32 ]; then mkdir $(BINDIR)/stm32; fi)
DUMMY:=$(shell if ! [ -d $(BINDIR)/usb ]; then mkdir $(BINDIR)/usb; fi)
DUMMY:=$(shell if ! [ -d $(BINDIR)/fatfs ]; then mkdir $(BINDIR)/fatfs; fi)

all: elf hex bin lss size

elf: $(BINDIR)/$(TARGET).elf
hex: $(BINDIR)/$(TARGET).hex
bin: $(BINDIR)/$(TARGET).bin
lss: $(BINDIR)/$(TARGET).lss

$(BINDIR)/$(TARGET).elf: $(OBJ)
	@echo "linking $@ ..."
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

$(BINDIR)/$(TARGET).hex: $(BINDIR)/$(TARGET).elf
	@echo "generating $@ ..."
	$(BIN) -O ihex $< $@
	
$(BINDIR)/$(TARGET).bin: $(BINDIR)/$(TARGET).elf
	@echo "generating $@ ..."
	$(BIN) -O binary $< $@
	
$(BINDIR)/$(TARGET).lss: $(BINDIR)/$(TARGET).elf
	@echo "generating $@ ..."
	$(OBJDUMP) -h -S $< > $@

$(BINDIR)/%.o: $(SRCDIR)/%.c
	@echo "compiling $< ..."
	$(CC) $(CFLAGS) $< -o $@

$(BINDIR)/%.o: $(LIBDIR)/%.c
	@echo "compiling $< ..."
	$(CC) $(CFLAGS) $< -o $@

	
phony: flash clean size

flash: all
	@echo "flashing ..."
	$(shell) st-link_cli -c SWD -P '$(BINDIR)/$(TARGET).bin' 0x08000000 -Rst
	
clean:
	@echo "cleaning up ..."
	@rm -r $(BINDIR)/*

size: elf
	$(SIZE) --format=sysv -d $(BINDIR)/$(TARGET).elf