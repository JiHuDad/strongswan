#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <library.h>
#include "../common/extsock_types.h"
#include "../common/extsock_errors.h"

START_TEST(test_basic_types)
{
    // 기본 타입들이 정의되었는지 확인
    extsock_error_t error = EXTSOCK_SUCCESS;
    ck_assert_int_eq(error, 0);
    
    extsock_command_type_t cmd = EXTSOCK_CMD_APPLY_CONFIG;
    ck_assert_int_eq(cmd, EXTSOCK_CMD_APPLY_CONFIG);
    
    extsock_event_type_t event = EXTSOCK_EVENT_TUNNEL_UP;
    ck_assert_int_eq(event, EXTSOCK_EVENT_TUNNEL_UP);
}
END_TEST

START_TEST(test_error_creation)
{
    // 에러 객체 생성 테스트
    extsock_error_info_t *error_info = extsock_error_create(EXTSOCK_ERROR_JSON_PARSE, "Test error");
    ck_assert_ptr_ne(error_info, NULL);
    
    ck_assert_int_eq(error_info->code, EXTSOCK_ERROR_JSON_PARSE);
    ck_assert_str_eq(error_info->message, "Test error");
    ck_assert_int_eq(error_info->severity, EXTSOCK_ERROR_SEVERITY_ERROR);
    
    extsock_error_destroy(error_info);
}
END_TEST

Suite *extsock_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("ExternalSocket");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_basic_types);
    tcase_add_test(tc_core, test_error_creation);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    // strongSwan 라이브러리 초기화
    library_init(NULL, "extsock-test");
    
    s = extsock_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    // strongSwan 라이브러리 정리
    library_deinit();

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 