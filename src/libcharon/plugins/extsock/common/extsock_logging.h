/*
 * Copyright (C) 2024 strongSwan Project
 * Enhanced Logging System for extsock Plugin
 */

#ifndef EXTSOCK_LOGGING_H_
#define EXTSOCK_LOGGING_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <syslog.h>

/**
 * 로그 레벨 정의
 */
typedef enum {
    EXTSOCK_LOG_LEVEL_TRACE = 0,    // 상세 추적 정보
    EXTSOCK_LOG_LEVEL_DEBUG,        // 디버그 정보
    EXTSOCK_LOG_LEVEL_INFO,         // 일반 정보
    EXTSOCK_LOG_LEVEL_WARN,         // 경고
    EXTSOCK_LOG_LEVEL_ERROR,        // 에러
    EXTSOCK_LOG_LEVEL_FATAL,        // 치명적 에러
    EXTSOCK_LOG_LEVEL_OFF           // 로깅 비활성화
} extsock_log_level_t;

/**
 * 로그 출력 대상
 */
typedef enum {
    EXTSOCK_LOG_TARGET_NONE     = 0,      // 로깅 없음
    EXTSOCK_LOG_TARGET_CONSOLE  = 1 << 0, // 콘솔 출력
    EXTSOCK_LOG_TARGET_FILE     = 1 << 1, // 파일 출력
    EXTSOCK_LOG_TARGET_SYSLOG   = 1 << 2, // syslog 출력
    EXTSOCK_LOG_TARGET_CALLBACK = 1 << 3, // 콜백 함수
    EXTSOCK_LOG_TARGET_ALL      = 0xFF    // 모든 출력
} extsock_log_target_t;

/**
 * 로그 포맷 옵션
 */
typedef enum {
    EXTSOCK_LOG_FORMAT_SIMPLE    = 0,  // 간단한 포맷
    EXTSOCK_LOG_FORMAT_DETAILED,       // 상세 포맷 (파일, 라인 포함)
    EXTSOCK_LOG_FORMAT_JSON,           // JSON 포맷
    EXTSOCK_LOG_FORMAT_CUSTOM          // 사용자 정의 포맷
} extsock_log_format_t;

/**
 * 로그 엔트리 구조체
 */
typedef struct extsock_log_entry_t {
    extsock_log_level_t level;      // 로그 레벨
    time_t timestamp;               // 타임스탬프
    uint32_t thread_id;             // 스레드 ID
    char component[32];             // 컴포넌트 이름
    char file[64];                  // 파일명
    int line;                       // 라인 번호
    char function[64];              // 함수명
    char message[512];              // 로그 메시지
    void *context_data;             // 컨텍스트 데이터
    size_t context_size;            // 컨텍스트 크기
} extsock_log_entry_t;

/**
 * 로그 콜백 함수 타입
 */
typedef void (*extsock_log_callback_t)(const extsock_log_entry_t *entry);

/**
 * 로그 포맷터 함수 타입
 */
typedef char *(*extsock_log_formatter_t)(const extsock_log_entry_t *entry);

/**
 * 로그 필터 함수 타입
 */
typedef bool (*extsock_log_filter_t)(const extsock_log_entry_t *entry);

/**
 * 로그 설정 구조체
 */
typedef struct extsock_log_config_t {
    extsock_log_level_t min_level;      // 최소 로그 레벨
    extsock_log_target_t targets;       // 출력 대상
    extsock_log_format_t format;        // 포맷 형식
    
    /* 파일 로깅 설정 */
    char log_file_path[256];            // 로그 파일 경로
    size_t max_file_size;               // 최대 파일 크기 (bytes)
    int max_backup_files;               // 최대 백업 파일 수
    bool auto_flush;                    // 자동 플러시 여부
    
    /* 콘솔 로깅 설정 */
    bool colored_output;                // 컬러 출력 여부
    bool timestamp_console;             // 콘솔에 타임스탬프 포함 여부
    
    /* 성능 설정 */
    bool async_logging;                 // 비동기 로깅 여부
    size_t buffer_size;                 // 로그 버퍼 크기
    uint32_t flush_interval_ms;         // 플러시 간격 (ms)
    
    /* 커스텀 설정 */
    extsock_log_callback_t callback;    // 사용자 정의 콜백
    extsock_log_formatter_t formatter;  // 사용자 정의 포맷터
    extsock_log_filter_t filter;        // 로그 필터
} extsock_log_config_t;

/**
 * 로거 인터페이스
 */
typedef struct extsock_logger_t {
    /**
     * 로그 메시지 출력
     */
    void (*log)(extsock_logger_t *this, extsock_log_level_t level,
                const char *component, const char *file, int line,
                const char *function, const char *format, ...);
    
    /**
     * 로그 엔트리 출력
     */
    void (*log_entry)(extsock_logger_t *this, const extsock_log_entry_t *entry);
    
    /**
     * 로그 레벨 설정
     */
    void (*set_level)(extsock_logger_t *this, extsock_log_level_t level);
    
    /**
     * 로그 대상 설정
     */
    void (*set_targets)(extsock_logger_t *this, extsock_log_target_t targets);
    
    /**
     * 로그 설정 변경
     */
    void (*configure)(extsock_logger_t *this, const extsock_log_config_t *config);
    
    /**
     * 로그 통계 조회
     */
    void (*get_statistics)(extsock_logger_t *this, uint64_t *total_logs,
                           uint64_t *logs_by_level[]);
    
    /**
     * 로그 버퍼 플러시
     */
    void (*flush)(extsock_logger_t *this);
    
    /**
     * 로거 해제
     */
    void (*destroy)(extsock_logger_t *this);
} extsock_logger_t;

/**
 * 컴포넌트별 로거 관리자
 */
typedef struct extsock_log_manager_t {
    /**
     * 컴포넌트 로거 등록
     */
    void (*register_component)(extsock_log_manager_t *this,
                               const char *component_name,
                               extsock_logger_t *logger);
    
    /**
     * 컴포넌트 로거 조회
     */
    extsock_logger_t *(*get_logger)(extsock_log_manager_t *this,
                                    const char *component_name);
    
    /**
     * 전체 로그 설정 적용
     */
    void (*configure_all)(extsock_log_manager_t *this,
                          const extsock_log_config_t *config);
    
    /**
     * 모든 로거 플러시
     */
    void (*flush_all)(extsock_log_manager_t *this);
    
    /**
     * 리소스 해제
     */
    void (*destroy)(extsock_log_manager_t *this);
} extsock_log_manager_t;

/**
 * 로깅 매크로들
 */
#define EXTSOCK_LOG(logger, level, fmt, ...) \
    do { \
        if (logger) { \
            (logger)->log(logger, level, EXTSOCK_COMPONENT_NAME, \
                         __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define EXTSOCK_LOG_TRACE(logger, fmt, ...) \
    EXTSOCK_LOG(logger, EXTSOCK_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)

#define EXTSOCK_LOG_DEBUG(logger, fmt, ...) \
    EXTSOCK_LOG(logger, EXTSOCK_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

#define EXTSOCK_LOG_INFO(logger, fmt, ...) \
    EXTSOCK_LOG(logger, EXTSOCK_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)

#define EXTSOCK_LOG_WARN(logger, fmt, ...) \
    EXTSOCK_LOG(logger, EXTSOCK_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)

#define EXTSOCK_LOG_ERROR(logger, fmt, ...) \
    EXTSOCK_LOG(logger, EXTSOCK_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define EXTSOCK_LOG_FATAL(logger, fmt, ...) \
    EXTSOCK_LOG(logger, EXTSOCK_LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)

/**
 * 함수 진입/종료 추적 매크로들
 */
#define EXTSOCK_LOG_FUNCTION_ENTER(logger) \
    EXTSOCK_LOG_TRACE(logger, "Entering function")

#define EXTSOCK_LOG_FUNCTION_EXIT(logger) \
    EXTSOCK_LOG_TRACE(logger, "Exiting function")

#define EXTSOCK_LOG_FUNCTION_EXIT_WITH_RESULT(logger, result) \
    EXTSOCK_LOG_TRACE(logger, "Exiting function with result: %d", result)

/**
 * 성능 측정 매크로들
 */
#define EXTSOCK_LOG_PERFORMANCE_START(logger, operation) \
    struct timespec _perf_start; \
    clock_gettime(CLOCK_MONOTONIC, &_perf_start); \
    EXTSOCK_LOG_DEBUG(logger, "Starting operation: %s", operation)

#define EXTSOCK_LOG_PERFORMANCE_END(logger, operation) \
    do { \
        struct timespec _perf_end; \
        clock_gettime(CLOCK_MONOTONIC, &_perf_end); \
        long _duration_ms = (_perf_end.tv_sec - _perf_start.tv_sec) * 1000 + \
                           (_perf_end.tv_nsec - _perf_start.tv_nsec) / 1000000; \
        EXTSOCK_LOG_INFO(logger, "Operation '%s' completed in %ld ms", \
                         operation, _duration_ms); \
    } while(0)

/**
 * 조건부 로깅 매크로
 */
#define EXTSOCK_LOG_IF(logger, condition, level, fmt, ...) \
    do { \
        if (condition) { \
            EXTSOCK_LOG(logger, level, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

/**
 * 한 번만 로깅하는 매크로
 */
#define EXTSOCK_LOG_ONCE(logger, level, fmt, ...) \
    do { \
        static bool _logged_once = false; \
        if (!_logged_once) { \
            EXTSOCK_LOG(logger, level, fmt, ##__VA_ARGS__); \
            _logged_once = true; \
        } \
    } while(0)

/**
 * 빈도 제한 로깅 매크로 (초당 최대 횟수)
 */
#define EXTSOCK_LOG_RATE_LIMITED(logger, level, max_per_sec, fmt, ...) \
    do { \
        static time_t _last_log_time = 0; \
        static int _log_count = 0; \
        time_t _current_time = time(NULL); \
        if (_current_time != _last_log_time) { \
            _last_log_time = _current_time; \
            _log_count = 0; \
        } \
        if (_log_count < max_per_sec) { \
            EXTSOCK_LOG(logger, level, fmt, ##__VA_ARGS__); \
            _log_count++; \
        } \
    } while(0)

/**
 * 헥사덤프 로깅 매크로
 */
#define EXTSOCK_LOG_HEXDUMP(logger, level, data, size, description) \
    do { \
        if (logger && data && size > 0) { \
            extsock_log_hexdump(logger, level, data, size, description, \
                                __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

/**
 * 전역 함수들
 */

/**
 * 로거 생성
 */
extsock_logger_t *extsock_logger_create(const extsock_log_config_t *config);

/**
 * 기본 로거 생성
 */
extsock_logger_t *extsock_logger_create_default();

/**
 * 로그 관리자 생성
 */
extsock_log_manager_t *extsock_log_manager_create();

/**
 * 전역 로그 관리자 조회
 */
extsock_log_manager_t *extsock_get_log_manager();

/**
 * 기본 로그 설정 조회
 */
extsock_log_config_t *extsock_get_default_log_config();

/**
 * 로그 레벨을 문자열로 변환
 */
const char *extsock_log_level_to_string(extsock_log_level_t level);

/**
 * 문자열을 로그 레벨로 변환
 */
extsock_log_level_t extsock_string_to_log_level(const char *level_str);

/**
 * 헥사덤프 로깅
 */
void extsock_log_hexdump(extsock_logger_t *logger, extsock_log_level_t level,
                         const void *data, size_t size, const char *description,
                         const char *file, int line, const char *function);

/**
 * 로그 엔트리 생성
 */
extsock_log_entry_t *extsock_log_entry_create(extsock_log_level_t level,
                                               const char *component,
                                               const char *file, int line,
                                               const char *function,
                                               const char *message);

/**
 * 로그 엔트리 해제
 */
void extsock_log_entry_destroy(extsock_log_entry_t *entry);

/**
 * 컴포넌트별 로거 조회 헬퍼
 */
#define EXTSOCK_GET_LOGGER(component) \
    extsock_get_log_manager()->get_logger(extsock_get_log_manager(), component)

/**
 * 컴포넌트별 로깅 매크로들
 */
#define EXTSOCK_PLUGIN_LOG_DEBUG(fmt, ...) \
    EXTSOCK_LOG_DEBUG(EXTSOCK_GET_LOGGER("plugin"), fmt, ##__VA_ARGS__)

#define EXTSOCK_PLUGIN_LOG_INFO(fmt, ...) \
    EXTSOCK_LOG_INFO(EXTSOCK_GET_LOGGER("plugin"), fmt, ##__VA_ARGS__)

#define EXTSOCK_PLUGIN_LOG_ERROR(fmt, ...) \
    EXTSOCK_LOG_ERROR(EXTSOCK_GET_LOGGER("plugin"), fmt, ##__VA_ARGS__)

#define EXTSOCK_JSON_LOG_DEBUG(fmt, ...) \
    EXTSOCK_LOG_DEBUG(EXTSOCK_GET_LOGGER("json_parser"), fmt, ##__VA_ARGS__)

#define EXTSOCK_SOCKET_LOG_DEBUG(fmt, ...) \
    EXTSOCK_LOG_DEBUG(EXTSOCK_GET_LOGGER("socket_adapter"), fmt, ##__VA_ARGS__)

#define EXTSOCK_CONFIG_LOG_DEBUG(fmt, ...) \
    EXTSOCK_LOG_DEBUG(EXTSOCK_GET_LOGGER("config_usecase"), fmt, ##__VA_ARGS__)

#define EXTSOCK_EVENT_LOG_DEBUG(fmt, ...) \
    EXTSOCK_LOG_DEBUG(EXTSOCK_GET_LOGGER("event_usecase"), fmt, ##__VA_ARGS__)

/**
 * 컴포넌트 이름 정의 (각 소스 파일에서 정의해야 함)
 */
#ifndef EXTSOCK_COMPONENT_NAME
#define EXTSOCK_COMPONENT_NAME "extsock"
#endif

#endif /** EXTSOCK_LOGGING_H_ @}*/ 