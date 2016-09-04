ARCH_FLAGS=-mcpu=cortex-m7
CC = arm-none-eabi-gcc
AR = arm-none-eabi-gcc-ar

# Remove at least flto and add -g when debugging
EMUCC_CFLAGS = $(ARCH_FLAGS) $(EMUCC_INCLUDE_FILES) -mthumb -flto -Wall -ffunction-sections -Ofast -c
EMUCC_LFLAGS = $(ARCH_FLAGS) --specs=nosys.specs -mthumb -flto -Ofast -Wl,-Map=./out_libemucc/libemucc.map,--gc-section

EMUDD_CFLAGS = $(ARCH_FLAGS) $(EMUDD_INCLUDE_FILES) -mthumb -flto -Wall -ffunction-sections -Ofast -c
EMUDD_LFLAGS = $(ARCH_FLAGS) --specs=nosys.specs -mthumb -flto -Ofast -Wl,-Map=./out_libemudd/libemudd.map,--gc-section

BL_CFLAGS = $(ARCH_FLAGS) $(BL_INCLUDES) -mthumb -Wall -static -ffunction-sections -O2 -c -DSTM32F756xx
BL_LFLAGS = $(ARCH_FLAGS) --specs=nosys.specs -mthumb -static -O2 -Wl,-Map=./out_bl/bl.map,--gc-section,-T ./link.ld

# NOTE: If building bootloader then change to ORIGIN = 0x08000000 in link.ld
# and also change the vector tables in system_stm32f7xx.c to 0x00000 (VECT_TAB_OFFSET).
TARGET_CFLAGS = $(ARCH_FLAGS) $(TARGET_INCLUDES) -mthumb -flto -Wall -static -ffunction-sections -Ofast -c -DSTM32F756xx
TARGET_LFLAGS = $(ARCH_FLAGS) --specs=nosys.specs -mthumb -flto -static -Ofast -Wl,-Map=./out_target/target.map,--gc-section,-T ./link.ld

EMUCC_INCLUDE_FILES := \
	-I./if \
	-I./emucc

EMUCC_LINK_FILES := \
	./out_libemucc/bus.o \
	./out_libemucc/cia.o \
	./out_libemucc/cpu.o \
	./out_libemucc/joy.o \
	./out_libemucc/sid.o \
	./out_libemucc/emuccif.o \
	./out_libemucc/tap.o \
	./out_libemucc/vic.o \
	./out_libemucc/key.o

EMUDD_INCLUDE_FILES := \
	-I./if \
	-I./emudd

EMUDD_LINK_FILES := \
	./out_libemudd/bus.o \
	./out_libemudd/via.o \
	./out_libemudd/cpu.o \
	./out_libemudd/fdd.o \
	./out_libemudd/emuddif.o \

BL_INCLUDES := \
	-I./if \
	-I./hw/config \
	-I./hw/core/cmsis \
	-I./hw/core/cmsis_boot \
	-I./hw/hal \
	-I./hw/irq \
	-I./hw/hal/Legacy \
	-I./hw/diag \
	-I./hw/drivers/sidbus \
	-I./hw/drivers/timer \
	-I./hw/drivers/disp \
	-I./hw/drivers/sdcard \
	-I./hw/drivers/crc \
	-I./hw/drivers/rng \
	-I./hw/drivers/sdram \
	-I./hw/drivers/adv7511 \
	-I./hw/drivers/usbd_ll \
	-I./hw/drivers/usbh_ll \
	-I./hw/drivers/joyst \
	-I./hw/mware/fatfs \
	-I./hw/mware/usb/host/core \
	-I./hw/mware/usb/host/hid \
	-I./hw/mware/usb/host/cust \
	-I./hw/mware/usb/device/core \
	-I./hw/mware/usb/device/cdc \
	-I./hw/mware/usb/device/cust \
	-I./hw/util/keybd \
	-I./hw/util/console \
	-I./hw/util/stage \
	-I./hw/rom \
	-I./hw/hostif \
	-I./hw/main \
	-I./hw/sm


BL_LINK_FILES := \
	./out_bl/startup_stm32f756xx.o \
	./out_bl/bl.o \
	./out_bl/sdcard.o \
	./out_bl/diskio.o \
	./out_bl/system_stm32f7xx.o \
	./out_bl/stm32f7xx_hal_flash.o \
	./out_bl/stm32f7xx_hal_flash_ex.o \
	./out_bl/stm32f7xx_hal_sd.o \
	./out_bl/stm32f7xx_hal.o \
	./out_bl/stm32f7xx_hal_gpio.o \
	./out_bl/stm32f7xx_hal_cortex.o \
	./out_bl/stm32f7xx_hal_pwr.o \
	./out_bl/stm32f7xx_hal_pwr_ex.o \
	./out_bl/stm32f7xx_hal_rcc.o \
	./out_bl/stm32f7xx_hal_rcc_ex.o \
	./out_bl/stm32f7xx_ll_sdmmc.o \
	./out_bl/ff.o \
	./out_bl/ccsbcs.o \

TARGET_INCLUDES := \
	-I./if \
	-I./hw/config \
	-I./hw/core/cmsis \
	-I./hw/core/cmsis_boot \
	-I./hw/hal \
	-I./hw/irq \
	-I./hw/hal/Legacy \
	-I./hw/diag \
	-I./hw/drivers/sidbus \
	-I./hw/drivers/timer \
	-I./hw/drivers/disp \
	-I./hw/drivers/sdcard \
	-I./hw/drivers/crc \
	-I./hw/drivers/rng \
	-I./hw/drivers/sdram \
	-I./hw/drivers/adv7511 \
	-I./hw/drivers/usbd_ll \
	-I./hw/drivers/usbh_ll \
	-I./hw/drivers/joyst \
	-I./hw/mware/fatfs \
	-I./hw/mware/usb/host/core \
	-I./hw/mware/usb/host/hid \
	-I./hw/mware/usb/host/cust \
	-I./hw/mware/usb/device/core \
	-I./hw/mware/usb/device/cdc \
	-I./hw/mware/usb/device/cust \
	-I./hw/util/keybd \
	-I./hw/util/console \
	-I./hw/util/stage \
	-I./hw/rom \
	-I./hw/hostif \
	-I./hw/main \
	-I./hw/sm

TARGET_LINK_FILES := \
	./out_target/config.o \
	./out_target/startup_stm32f756xx.o \
	./out_target/sidbus.o \
	./out_target/timer.o \
	./out_target/sdcard.o \
	./out_target/crc.o \
	./out_target/rng.o \
	./out_target/joyst.o \
	./out_target/diskio.o \
	./out_target/sdram.o \
	./out_target/adv7511.o \
	./out_target/disp.o \
	./out_target/console.o \
	./out_target/keybd.o \
	./out_target/stage.o \
	./out_target/diag.o \
	./out_target/usbh_conf.o \
	./out_target/usbd_cdc_if.o \
	./out_target/usbd_desc.o \
	./out_target/usbd_conf.o \
	./out_target/system_stm32f7xx.o \
	./out_target/stm32f7xx_hal_sd.o \
	./out_target/stm32f7xx_hal.o \
	./out_target/stm32f7xx_hal_dma2d.o \
	./out_target/stm32f7xx_hal_ltdc.o \
	./out_target/stm32f7xx_hal_ltdc_ex.o \
	./out_target/stm32f7xx_hal_sdram.o \
	./out_target/stm32f7xx_hal_pcd.o \
	./out_target/stm32f7xx_hal_pcd_ex.o \
	./out_target/stm32f7xx_hal_gpio.o \
	./out_target/stm32f7xx_hal_cortex.o \
	./out_target/stm32f7xx_hal_pwr.o \
	./out_target/stm32f7xx_hal_pwr_ex.o \
	./out_target/stm32f7xx_hal_hcd.o \
	./out_target/stm32f7xx_hal_rcc.o \
	./out_target/stm32f7xx_hal_i2c.o \
	./out_target/stm32f7xx_hal_rcc_ex.o \
	./out_target/stm32f7xx_hal_i2c_ex.o \
	./out_target/stm32f7xx_hal_crc.o \
	./out_target/stm32f7xx_hal_crc_ex.o \
	./out_target/stm32f7xx_hal_rng.o \
	./out_target/stm32f7xx_ll_fmc.o \
	./out_target/stm32f7xx_ll_sdmmc.o \
	./out_target/stm32f7xx_ll_usb.o \
	./out_target/stm32f7xx_hal_dma.o \
	./out_target/stm32f7xx_hal_tim.o \
	./out_target/irq.o \
	./out_target/usbh_core.o \
	./out_target/usbh_ctlreq.o \
	./out_target/usbh_ioreq.o \
	./out_target/usbh_pipes.o \
	./out_target/usbh_hid.o \
	./out_target/usbh_hid_keybd.o \
	./out_target/usbh_hid_parser.o \
	./out_target/usbd_core.o \
	./out_target/usbd_ctlreq.o \
	./out_target/usbd_ioreq.o \
	./out_target/usbd_cdc.o \
	./out_target/ff.o \
	./out_target/ccsbcs.o \
	./out_target/hostif.o \
	./out_target/main.o \
	./out_target/sm.o \
	./out_target/romcc.o \
	./out_target/romdd.o

EMUCC = libemucc
EMUDD = libemudd
BL = bootloader
TARGET = target

all: $(EMUCC) $(EMUDD) $(TARGET)

$(EMUCC):
	@mkdir ./out_libemucc 2>/dev/null; true
	@echo Compiling emucc...
	$(CC) $(EMUCC_CFLAGS) -o out_libemucc/emuccif.o ./emucc/emuccif.c
	$(CC) $(EMUCC_CFLAGS) -o out_libemucc/bus.o ./emucc/bus.c
	$(CC) $(EMUCC_CFLAGS) -o out_libemucc/cia.o ./emucc/cia.c
	$(CC) $(EMUCC_CFLAGS) -o out_libemucc/cpu.o ./emucc/cpu.c
	$(CC) $(EMUCC_CFLAGS) -o out_libemucc/joy.o ./emucc/joy.c
	$(CC) $(EMUCC_CFLAGS) -o out_libemucc/sid.o ./emucc/sid.c
	$(CC) $(EMUCC_CFLAGS) -o out_libemucc/tap.o ./emucc/tap.c
	$(CC) $(EMUCC_CFLAGS) -DSCREEN_X2 -DHAVE_BORDERS -o out_libemucc/vic.o ./emucc/vic.c
	$(CC) $(EMUCC_CFLAGS) -o out_libemucc/key.o ./emucc/key.c

	@echo Linking...
	$(AR) rcs out_libemucc/libemucc.a $(EMUCC_LINK_FILES)

$(EMUDD):
	@mkdir ./out_libemudd 2>/dev/null; true
	@echo Compiling emudd...
	$(CC) $(EMUDD_CFLAGS) -o out_libemudd/bus.o ./emudd/bus.c
	$(CC) $(EMUDD_CFLAGS) -o out_libemudd/via.o ./emudd/via.c
	$(CC) $(EMUDD_CFLAGS) -o out_libemudd/cpu.o ./emudd/cpu.c
	$(CC) $(EMUDD_CFLAGS) -o out_libemudd/fdd.o ./emudd/fdd.c
	$(CC) $(EMUDD_CFLAGS) -o out_libemudd/emuddif.o ./emudd/emuddif.c

	@echo Linking...
	$(AR) rcs out_libemudd/libemudd.a $(EMUDD_LINK_FILES)

$(BL):
	@mkdir ./out_bl 2>/dev/null; true
	@echo Compiling bootloader...
	$(CC) $(BL_CFLAGS) -o out_bl/sdcard.o ./hw/drivers/sdcard/sdcard.c
	$(CC) $(BL_CFLAGS) -o out_bl/diskio.o ./hw/drivers/sdcard/diskio.c
	$(CC) $(BL_CFLAGS) -o out_bl/disp.o ./hw/drivers/disp/disp.c
	$(CC) $(BL_CFLAGS) -o out_bl/system_stm32f7xx.o ./hw/core/cmsis_boot/system_stm32f7xx.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_hal_sd.o ./hw/hal/stm32f7xx_hal_sd.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_hal.o ./hw/hal/stm32f7xx_hal.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_hal_flash.o ./hw/hal/stm32f7xx_hal_flash.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_hal_flash_ex.o ./hw/hal/stm32f7xx_hal_flash_ex.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_hal_ltdc.o ./hw/hal/stm32f7xx_hal_ltdc.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_hal_gpio.o ./hw/hal/stm32f7xx_hal_gpio.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_hal_cortex.o ./hw/hal/stm32f7xx_hal_cortex.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_hal_pwr.o ./hw/hal/stm32f7xx_hal_pwr.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_hal_pwr_ex.o ./hw/hal/stm32f7xx_hal_pwr_ex.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_hal_rcc.o ./hw/hal/stm32f7xx_hal_rcc.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_hal_rcc_ex.o ./hw/hal/stm32f7xx_hal_rcc_ex.c
	$(CC) $(BL_CFLAGS) -o out_bl/stm32f7xx_ll_sdmmc.o ./hw/hal/stm32f7xx_ll_sdmmc.c
	$(CC) $(BL_CFLAGS) -o out_bl/irq.o ./hw/irq/irq.c
	$(CC) $(BL_CFLAGS) -o out_bl/ff.o ./hw/mware/fatfs/ff.c
	$(CC) $(BL_CFLAGS) -o out_bl/ccsbcs.o ./hw/mware/fatfs/ccsbcs.c
	$(CC) $(BL_CFLAGS) -o out_bl/bl.o ./bl/bl.c
	$(CC) $(BL_CFLAGS) -o out_bl/startup_stm32f756xx.o ./hw/core/cmsis_boot/startup/startup_stm32f756xx.s

	@echo Linking...
	$(CC) $(BL_LFLAGS) -o out_bl/bl.elf $(BL_LINK_FILES)


$(TARGET):
	@mkdir ./out_target 2>/dev/null; true
	@echo Compiling target...
	$(CC) $(TARGET_CFLAGS) -DUSE_HSE -o out_target/config.o ./hw/config/config.c
	$(CC) $(TARGET_CFLAGS) -o out_target/sidbus.o ./hw/drivers/sidbus/sidbus.c
	$(CC) $(TARGET_CFLAGS) -o out_target/timer.o ./hw/drivers/timer/timer.c
	$(CC) $(TARGET_CFLAGS) -o out_target/sdcard.o ./hw/drivers/sdcard/sdcard.c
	$(CC) $(TARGET_CFLAGS) -o out_target/diskio.o ./hw/drivers/sdcard/diskio.c
	$(CC) $(TARGET_CFLAGS) -o out_target/crc.o ./hw/drivers/crc/crc.c
	$(CC) $(TARGET_CFLAGS) -o out_target/rng.o ./hw/drivers/rng/rng.c
	$(CC) $(TARGET_CFLAGS) -o out_target/joyst.o ./hw/drivers/joyst/joyst.c
	$(CC) $(TARGET_CFLAGS) -o out_target/sdram.o ./hw/drivers/sdram/sdram.c
	$(CC) $(TARGET_CFLAGS) -o out_target/adv7511.o ./hw/drivers/adv7511/adv7511.c
	$(CC) $(TARGET_CFLAGS) -o out_target/disp.o ./hw/drivers/disp/disp.c
	$(CC) $(TARGET_CFLAGS) -o out_target/console.o ./hw/util/console/console.c
	$(CC) $(TARGET_CFLAGS) -o out_target/keybd.o ./hw/util/keybd/keybd.c
	$(CC) $(TARGET_CFLAGS) -DSCREEN_X2 -o out_target/stage.o ./hw/util/stage/stage.c
	$(CC) $(TARGET_CFLAGS) -o out_target/diag.o ./hw/diag/diag.c
	$(CC) $(TARGET_CFLAGS) -o out_target/system_stm32f7xx.o ./hw/core/cmsis_boot/system_stm32f7xx.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_sd.o ./hw/hal/stm32f7xx_hal_sd.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal.o ./hw/hal/stm32f7xx_hal.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_dma2d.o ./hw/hal/stm32f7xx_hal_dma2d.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_ltdc.o ./hw/hal/stm32f7xx_hal_ltdc.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_ltdc_ex.o ./hw/hal/stm32f7xx_hal_ltdc_ex.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_sdram.o ./hw/hal/stm32f7xx_hal_sdram.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_pcd.o ./hw/hal/stm32f7xx_hal_pcd.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_pcd_ex.o ./hw/hal/stm32f7xx_hal_pcd_ex.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_gpio.o ./hw/hal/stm32f7xx_hal_gpio.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_cortex.o ./hw/hal/stm32f7xx_hal_cortex.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_pwr.o ./hw/hal/stm32f7xx_hal_pwr.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_crc.o ./hw/hal/stm32f7xx_hal_crc.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_pwr_ex.o ./hw/hal/stm32f7xx_hal_pwr_ex.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_crc_ex.o ./hw/hal/stm32f7xx_hal_crc_ex.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_hcd.o ./hw/hal/stm32f7xx_hal_hcd.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_rcc.o ./hw/hal/stm32f7xx_hal_rcc.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_uart.o ./hw/hal/stm32f7xx_hal_uart.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_i2c.o ./hw/hal/stm32f7xx_hal_i2c.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_rcc_ex.o ./hw/hal/stm32f7xx_hal_rcc_ex.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_i2c_ex.o ./hw/hal/stm32f7xx_hal_i2c_ex.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_rng.o ./hw/hal/stm32f7xx_hal_rng.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_ll_fmc.o ./hw/hal/stm32f7xx_ll_fmc.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_ll_sdmmc.o ./hw/hal/stm32f7xx_ll_sdmmc.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_ll_usb.o ./hw/hal/stm32f7xx_ll_usb.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_dma.o ./hw/hal/stm32f7xx_hal_dma.c
	$(CC) $(TARGET_CFLAGS) -o out_target/stm32f7xx_hal_tim.o ./hw/hal/stm32f7xx_hal_tim.c
	$(CC) $(TARGET_CFLAGS) -o out_target/irq.o ./hw/irq/irq.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbh_core.o ./hw/mware/usb/host/core/usbh_core.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbh_ctlreq.o ./hw/mware/usb/host/core/usbh_ctlreq.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbh_ioreq.o ./hw/mware/usb/host/core/usbh_ioreq.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbh_pipes.o ./hw/mware/usb/host/core/usbh_pipes.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbh_hid.o ./hw/mware/usb/host/hid/usbh_hid.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbh_hid_keybd.o ./hw/mware/usb/host/hid/usbh_hid_keybd.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbh_hid_parser.o ./hw/mware/usb/host/hid/usbh_hid_parser.c
	$(CC) $(TARGET_CFLAGS) -DUSE_USB_HS -o out_target/usbh_conf.o ./hw/drivers/usbh_ll/usbh_conf.c
	$(CC) $(TARGET_CFLAGS) -DUSE_USB_FS -o out_target/usbd_conf.o ./hw/drivers/usbd_ll/usbd_conf.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbd_core.o ./hw/mware/usb/device/core/usbd_core.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbd_ctlreq.o ./hw/mware/usb/device/core/usbd_ctlreq.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbd_ioreq.o ./hw/mware/usb/device/core/usbd_ioreq.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbd_cdc.o ./hw/mware/usb/device/cdc/usbd_cdc.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbd_cdc_if.o ./hw/mware/usb/device/cust/usbd_cdc_if.c
	$(CC) $(TARGET_CFLAGS) -o out_target/usbd_desc.o ./hw/mware/usb/device/cust/usbd_desc.c
	$(CC) $(TARGET_CFLAGS) -o out_target/ff.o ./hw/mware/fatfs/ff.c
	$(CC) $(TARGET_CFLAGS) -o out_target/ccsbcs.o ./hw/mware/fatfs/ccsbcs.c
	$(CC) $(TARGET_CFLAGS) -DUSE_CRC -o out_target/hostif.o ./hw/hostif/hostif.c
	$(CC) $(TARGET_CFLAGS) -o out_target/main.o ./hw/main/main.c
	$(CC) $(TARGET_CFLAGS) -o out_target/sm.o ./hw/sm/sm.c
	$(CC) $(TARGET_CFLAGS) -o out_target/romcc.o ./hw/rom/romcc.c
	$(CC) $(TARGET_CFLAGS) -o out_target/romdd.o ./hw/rom/romdd.c
	$(CC) $(TARGET_CFLAGS) -o out_target/startup_stm32f756xx.o ./hw/core/cmsis_boot/startup/startup_stm32f756xx.s

	@echo Linking...
	$(CC) $(TARGET_LFLAGS) -o out_target/target.elf $(TARGET_LINK_FILES) -L./out_libemucc -lemucc -L./out_libemudd -lemudd

	@echo Create binary...
	arm-none-eabi-objcopy -O binary out_target/target.elf out_target/target.bin

clean:
	rm -rf ./out_target ./out_bl ./out_libemucc ./out_libemudd

