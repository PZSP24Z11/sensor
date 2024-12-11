APPLICATION = pzsp2_sensor
BOARD ?= native
RIOTBASE ?= $(CURDIR)/../RIOT

# Constants
SAMPLE_RATE = 1000
NUM_READINGS = 2
READING_LEN = 6

# development process:
DEVELHELP = 1
QUIET ?= 1

# Modules to include:
USEMODULE += sock_udp
USEMODULE += shell
USEMODULE += shell_cmds_default
USEMODULE += periph_gpio
USEMODULE += ztimer_usec
USEMODULE += ztimer_msec
USEMODULE += dht
USEMODULE += pcd8544
USEMODULE += netdev_default
USEMODULE += auto_init_gnrc_netif 	# Comment to make LCD working
USEMODULE += gnrc_ipv6_default
USEPKG 	  += wolfssl
USEMODULE += wolfcrypt
USEMODULE += wolfssl
USEMODULE += wolfssl_dtls
USEMODULE += wolfcrypt_ecc

FEATURES_OPTIONAL += periph_rtc
CFLAGS += -DDTLS_DEFAULT_PORT=$(DTLS_PORT) -DDTLS_WOLFSSL -Wno-unused-parameter -Wno-unused-variable
CFLAGS += -DTHREAD_STACKSIZE_MAIN=\(10*THREAD_STACKSIZE_DEFAULT\)
CFLAGS += -DNUM_READINGS=$(NUM_READINGS)
CFLAGS += -DREADING_LEN=$(READING_LEN)
CFLAGS += -DSAMPLE_RATE=$(SAMPLE_RATE)

# used without a gpiochip
ifneq (,$(filter native native64,$(BOARD)))
 USEMODULE += periph_gpio_mock
endif

include $(RIOTBASE)/Makefile.include
