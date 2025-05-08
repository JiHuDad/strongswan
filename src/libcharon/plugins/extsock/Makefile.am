# 플러그인 라이브러리의 이름입니다. libextsock.la 형태로 빌드됩니다.
# libcharon 플러그인이므로 charon_plugin_LTLIBRARIES를 사용할 수도 있지만,
# plugin_LTLIBRARIES도 일반적으로 잘 동작합니다.
# 기존 charon 플러그인들의 Makefile.am을 참고하여 더 적합한 변수명을 선택할 수 있습니다.
plugin_LTLIBRARIES = libextsock.la

# libextsock.la를 빌드하기 위한 소스 파일 목록입니다.
# extsock_plugin.c가 주 소스파일이고, extsock.h는 헤더로 가정합니다.
libextsock_la_SOURCES = extsock_plugin.c

# 플러그인에 필요한 C 컴파일러 플래그가 있다면 여기에 추가합니다.
# 예: libextsock_la_CFLAGS = -DSOME_MACRO $(AM_CFLAGS)
# AM_CFLAGS는 전역 CFLAGS를 포함하기 위해 추가할 수 있습니다.
libextsock_la_CFLAGS = $(AM_CFLAGS)

# 플러그인이 링크해야 할 다른 라이브러리가 있다면 여기에 추가합니다.
# 외부 cJSON 라이브러리를 사용하므로 -lcjson 을 추가합니다.
libextsock_la_LIBADD = -lcjson

# Libtool 라이브러리를 빌드할 때 사용되는 LDFLAGS입니다.
# -module: 공유 모듈로 빌드하도록 지정합니다.
# -avoid-version: 라이브러리 이름에 버전 번호를 붙이지 않습니다 (예: .so.0.0.0).
# -no-undefined: 라이브러리 내에 정의되지 않은 심볼이 없도록 합니다.
libextsock_la_LDFLAGS = -module -avoid-version -no-undefined

# 플러그인이 설치될 디렉토리입니다.
# automake는 plugin_LTLIBRARIES를 $(plugindir)에 설치하려고 시도합니다.
# plugindir은 configure.ac에서 정의됩니다.
# charon 플러그인의 경우 $(charonplugindir) 변수가 사용될 수도 있습니다.
# configure.ac 및 다른 charon 플러그인의 Makefile.am을 확인하여 정확한 변수를 사용하는 것이 좋습니다.
# 일단 $(plugindir)을 기본으로 사용합니다. 