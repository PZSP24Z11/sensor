APPLICATION = pzsp2_sensor

BOARD ?= native
#BOARD ?= nucleo-f439zi
#PROGRAMMER ?= cpy2remed

RIOTBASE ?= $(CURDIR)/../RIOT

NUM_READINGS = 2
READING_LEN = 5

# GNRC network stack
USEMODULE += netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += sock_udp

# shell
USEMODULE += shell
USEMODULE += shell_cmds_default

# wolfssl
USEPKG += wolfssl
USEMODULE += wolfcrypt
USEMODULE += wolfssl
USEMODULE += wolfssl_dtls

# Ciphersuit module
USEMODULE += wolfcrypt_ecc

FEATURES_OPTIONAL += periph_rtc

CFLAGS += -DDTLS_DEFAULT_PORT=$(DTLS_PORT) -DDTLS_WOLFSSL -Wno-unused-parameter -Wno-unused-variable

# A larger stack size is required if using ECC or RSA
CFLAGS += -DTHREAD_STACKSIZE_MAIN=\(10*THREAD_STACKSIZE_DEFAULT\)

# Number of sensor readings
CFLAGS += -DNUM_READINGS=$(NUM_READINGS)
CFLAGS += -DREADING_LEN=$(READING_LEN)

DEVELHELP ?= 1
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
