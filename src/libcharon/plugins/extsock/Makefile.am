# 플러그인 라이브러리의 이름입니다. libextsock.la 형태로 빌드됩니다.
# libcharon 플러그인이므로 charon_plugin_LTLIBRARIES를 사용할 수도 있지만,
# plugin_LTLIBRARIES도 일반적으로 잘 동작합니다.
# 기존 charon 플러그인들의 Makefile.am을 참고하여 더 적합한 변수명을 선택할 수 있습니다.

AM_CPPFLAGS = \
    -I$(top_srcdir)/src/libstrongswan \
    -I$(top_srcdir)/src/libcharon \
    $(EXTSOCK_CFLAGS)

AM_CFLAGS = \
    $(PLUGIN_CFLAGS)

if MONOLITHIC
noinst_LTLIBRARIES = libstrongswan-extsock.la
else
plugin_LTLIBRARIES = libstrongswan-extsock.la
endif

# Clean Architecture 모든 소스 파일 포함
libstrongswan_extsock_la_SOURCES = \
    extsock_plugin.c \
    common/extsock_types.h \
    common/extsock_common.h \
    interfaces/extsock_config_repository.h \
    interfaces/extsock_event_publisher.h \
    interfaces/extsock_command_handler.h \
    adapters/json/extsock_json_parser.h \
    adapters/json/extsock_json_parser.c \
    adapters/socket/extsock_socket_adapter.h \
    adapters/socket/extsock_socket_adapter.c \
    adapters/strongswan/extsock_strongswan_adapter.h \
    adapters/strongswan/extsock_strongswan_adapter.c \
    adapters/crypto/extsock_cert_loader.h \
    adapters/crypto/extsock_cert_loader.c \
    domain/extsock_config_entity.h \
    domain/extsock_config_entity.c \
    usecases/extsock_config_usecase.h \
    usecases/extsock_config_usecase.c \
    usecases/extsock_event_usecase.h \
    usecases/extsock_event_usecase.c \
    interfaces/extsock_failover_manager.h \
    usecases/extsock_failover_manager.c

# cJSON 라이브러리 링크
libstrongswan_extsock_la_LIBADD = $(EXTSOCK_LIBS)

# Libtool 라이브러리를 빌드할 때 사용되는 LDFLAGS입니다.
# -module: 공유 모듈로 빌드하도록 지정합니다.
# -avoid-version: 라이브러리 이름에 버전 번호를 붙이지 않습니다 (예: .so.0.0.0).
# -no-undefined: 라이브러리 내에 정의되지 않은 심볼이 없도록 합니다.
libstrongswan_extsock_la_LDFLAGS = -module -avoid-version -no-undefined

# 플러그인이 설치될 디렉토리입니다.
# automake는 plugin_LTLIBRARIES를 $(plugindir)에 설치하려고 시도합니다.
# plugindir은 configure.ac에서 정의됩니다.
# charon 플러그인의 경우 $(charonplugindir) 변수가 사용될 수도 있습니다.
# configure.ac 및 다른 charon 플러그인의 Makefile.am을 확인하여 정확한 변수를 사용하는 것이 좋습니다.
# 일단 $(plugindir)을 기본으로 사용합니다. 