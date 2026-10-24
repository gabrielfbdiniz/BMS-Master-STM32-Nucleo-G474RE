# ==============================================================================
# Makefile para o BMS Master V0 (Formula ITA) — Firmware Monorepo
# Suporta:
#   1. Compilação para ARM Cortex-M4F (STM32G474RE / STM32F411)
#   2. Execução da Simulação SIL Nativa no PC (via Clang ou GCC)
# ==============================================================================

CC_ARM ?= "C:/ST/STM32CubeIDE_1.18.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344/tools/bin/arm-none-eabi-gcc.exe"
SIZE_ARM ?= "C:/ST/STM32CubeIDE_1.18.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344/tools/bin/arm-none-eabi-size.exe"
CLANG_HOST ?= "C:/Users/gabri/AppData/Local/Microsoft/WinGet/Packages/MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe"

CFLAGS_ARM = -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb \
             -O2 -Wall -Wextra -std=c99 \
             -Icore -Imock -Ihal_interface

SRC_CORE = core/alg_fuse_detect.c \
           core/alg_soc.c \
           core/fsm_master.c

SRC_MOCK = mock/bms_mock_battery.c \
           mock/drive_cycle.c

SRC_ALL = $(SRC_CORE) $(SRC_MOCK)
OBJS_ARM = $(SRC_ALL:.c=.o)

all: arm_check sil_build

arm_check: $(OBJS_ARM)
	@echo ----------------------------------------------------------------------
	@echo [OK] Modulos de firmware compilados para ARM Cortex-M4F!
	@echo ----------------------------------------------------------------------
	$(SIZE_ARM) $(OBJS_ARM)

%.o: %.c
	$(CC_ARM) $(CFLAGS_ARM) -c $< -o $@

sil_build:
	@echo ----------------------------------------------------------------------
	@echo [BUILD] Compilando executavel de simulacao SIL nativo...
	@echo ----------------------------------------------------------------------
	$(CLANG_HOST) -O2 -Wall -Wextra -Icore -Imock \
		tests_sil/main_sil_runner.c $(SRC_ALL) \
		-o tests_sil/sil_runner.exe -lm
	@echo [OK] Executavel gerado: tests_sil/sil_runner.exe

clean:
	rm -f $(OBJS_ARM) tests_sil/sil_runner.exe *.o
