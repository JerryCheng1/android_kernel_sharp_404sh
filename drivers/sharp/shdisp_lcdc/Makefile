ifneq (, $(filter F%, $(SH_BUILD_ID)))
EXTRA_CFLAGS += -DSHDISP_FACTORY_MODE_ENABLE
endif

obj-$(CONFIG_SHDISP) += shdisp_kerl.o

obj-$(CONFIG_SHDISP) += shdisp_system.o

obj-$(CONFIG_SHDISP) += shdisp_bl71y6.o

obj-$(CONFIG_SHDISP) += shdisp_pm.o
obj-$(CONFIG_SHDISP) += shdisp_panel.o

ifeq ($(CONFIG_SHDISP_PANEL_ANDY),y)
obj-$(CONFIG_SHDISP) += shdisp_andy.o
endif
ifeq ($(CONFIG_SHDISP_PANEL_CARIN),y)
obj-$(CONFIG_SHDISP) += shdisp_carin.o
endif
ifeq ($(CONFIG_SHDISP_PANEL_GEMINI),y)
obj-$(CONFIG_SHDISP) += shdisp_gemini.o
endif

obj-$(CONFIG_SHDISP) += shdisp_clmr.o
obj-$(CONFIG_SHDISP) += shdisp_clmr_usr.o

ifeq ($(CONFIG_SHLCDC_LED_BD2802GU),y)
obj-$(CONFIG_SHDISP) += shdisp_bd2802gu.o
endif

obj-$(CONFIG_SHDISP) += shdisp_dbg.o
EXTRA_CFLAGS    += -DSH_BUILD_ID=\"$(SH_BUILD_ID)\"
EXTRA_CFLAGS    += -DUSE_LINUX
