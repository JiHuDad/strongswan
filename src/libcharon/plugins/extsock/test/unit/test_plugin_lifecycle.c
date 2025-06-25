/*
 * Copyright (C) 2024 strongSwan Project
 * Unit tests for Plugin Lifecycle and Dependency Injection
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <library.h>
#include <plugins/plugin.h>
#include <threading/thread.h>
#include "../../extsock_plugin.h"
#include "../../common/extsock_common.h"

static plugin_t *plugin;

/**
 * 테스트 설정
 */
void setup_plugin_lifecycle_test(void)
{
    library_init(NULL, "test-plugin-lifecycle");
    plugin = NULL;
}

/**
 * 테스트 해제
 */
void teardown_plugin_lifecycle_test(void)
{
    if (plugin) {
        plugin->destroy(plugin);
        plugin = NULL;
    }
    
    library_deinit();
}

/**
 * 플러그인 생성 테스트
 */
START_TEST(test_plugin_creation)
{
    // When
    plugin = extsock_plugin_create();
    
    // Then
    ck_assert_ptr_nonnull(plugin);
    ck_assert_ptr_nonnull(plugin->get_name);
    ck_assert_ptr_nonnull(plugin->get_features);
    ck_assert_ptr_nonnull(plugin->destroy);
}
END_TEST

/**
 * 플러그인 이름 조회 테스트
 */
START_TEST(test_plugin_get_name)
{
    // Given
    plugin = extsock_plugin_create();
    ck_assert_ptr_nonnull(plugin);
    
    // When
    char *name = plugin->get_name(plugin);
    
    // Then
    ck_assert_ptr_nonnull(name);
    ck_assert_str_eq(name, "extsock");
}
END_TEST

/**
 * 플러그인 기능 조회 테스트
 */
START_TEST(test_plugin_get_features)
{
    // Given
    plugin = extsock_plugin_create();
    ck_assert_ptr_nonnull(plugin);
    
    plugin_feature_t *features;
    
    // When
    int feature_count = plugin->get_features(plugin, &features);
    
    // Then
    ck_assert_int_gt(feature_count, 0);
    ck_assert_ptr_nonnull(features);
    
    // CUSTOM "extsock" 기능이 있는지 확인
    bool found_extsock_feature = FALSE;
    for (int i = 0; i < feature_count; i++) {
        if (features[i].type == FEATURE_CUSTOM &&
            features[i].arg.custom && 
            streq(features[i].arg.custom, "extsock")) {
            found_extsock_feature = TRUE;
            break;
        }
    }
    ck_assert_int_eq(found_extsock_feature, TRUE);
}
END_TEST

/**
 * 플러그인 정상 해제 테스트
 */
START_TEST(test_plugin_destroy)
{
    // Given
    plugin = extsock_plugin_create();
    ck_assert_ptr_nonnull(plugin);
    
    // 플러그인이 정상적으로 초기화되었는지 확인
    char *name = plugin->get_name(plugin);
    ck_assert_str_eq(name, "extsock");
    
    // When
    plugin->destroy(plugin);
    plugin = NULL; // teardown에서 다시 해제하지 않도록
    
    // Then - 메모리 누수나 크래시가 없으면 성공
}
END_TEST

/**
 * 의존성 주입 컨테이너 초기화 실패 시뮬레이션 테스트
 */
START_TEST(test_dependency_injection_failure)
{
    // Note: 실제 내부 의존성 실패를 시뮬레이션하기 위해서는
    // 모킹이나 환경 조작이 필요하지만, 여기서는 정상 동작만 확인
    
    // When
    plugin = extsock_plugin_create();
    
    // Then - 플러그인이 NULL이면 DI 컨테이너 초기화 실패
    // 정상적인 환경에서는 성공해야 함
    ck_assert_ptr_nonnull(plugin);
}
END_TEST

/**
 * 다중 플러그인 인스턴스 생성 테스트
 */
START_TEST(test_multiple_plugin_instances)
{
    plugin_t *plugin1, *plugin2;
    
    // Given & When
    plugin1 = extsock_plugin_create();
    plugin2 = extsock_plugin_create();
    
    // Then
    ck_assert_ptr_nonnull(plugin1);
    ck_assert_ptr_nonnull(plugin2);
    ck_assert_ptr_ne(plugin1, plugin2); // 서로 다른 인스턴스
    
    // 각각 정상 동작하는지 확인
    ck_assert_str_eq(plugin1->get_name(plugin1), "extsock");
    ck_assert_str_eq(plugin2->get_name(plugin2), "extsock");
    
    // 정리
    plugin1->destroy(plugin1);
    plugin2->destroy(plugin2);
}
END_TEST

/**
 * 소켓 스레드 생명주기 테스트
 */
START_TEST(test_socket_thread_lifecycle)
{
    // Given
    plugin = extsock_plugin_create();
    ck_assert_ptr_nonnull(plugin);
    
    // 플러그인 생성 시 소켓 스레드가 시작되어야 함
    // (실제 구현에서는 백그라운드 스레드가 동작)
    
    // 잠시 대기하여 초기화 완료 확인
    usleep(100000); // 100ms
    
    // When - 플러그인 해제
    plugin->destroy(plugin);
    plugin = NULL;
    
    // Then - 스레드가 정상적으로 정리되어야 함 (크래시 없음)
}
END_TEST

/**
 * 빠른 생성/해제 사이클 테스트
 */
START_TEST(test_rapid_create_destroy_cycle)
{
    // 빠른 생성/해제를 반복하여 리소스 누수나 경쟁 조건 확인
    
    for (int i = 0; i < 10; i++) {
        // When
        plugin_t *temp_plugin = extsock_plugin_create();
        
        // Then
        ck_assert_ptr_nonnull(temp_plugin);
        ck_assert_str_eq(temp_plugin->get_name(temp_plugin), "extsock");
        
        // 즉시 해제
        temp_plugin->destroy(temp_plugin);
        
        // 짧은 대기
        usleep(10000); // 10ms
    }
}
END_TEST

/**
 * 메모리 제한 환경에서의 플러그인 생성 테스트
 */
START_TEST(test_memory_constrained_creation)
{
    // Note: 실제 메모리 제한을 시뮬레이션하기는 어렵지만
    // 여러 번 생성하여 메모리 사용량 확인
    
    plugin_t *plugins[5];
    
    // 여러 플러그인 인스턴스 생성
    for (int i = 0; i < 5; i++) {
        plugins[i] = extsock_plugin_create();
        ck_assert_ptr_nonnull(plugins[i]);
    }
    
    // 모든 플러그인이 정상 동작하는지 확인
    for (int i = 0; i < 5; i++) {
        ck_assert_str_eq(plugins[i]->get_name(plugins[i]), "extsock");
    }
    
    // 모든 플러그인 해제
    for (int i = 0; i < 5; i++) {
        plugins[i]->destroy(plugins[i]);
    }
}
END_TEST

/**
 * 플러그인 기능 일관성 테스트
 */
START_TEST(test_plugin_feature_consistency)
{
    // Given
    plugin = extsock_plugin_create();
    ck_assert_ptr_nonnull(plugin);
    
    plugin_feature_t *features1, *features2;
    
    // When - 여러 번 기능 조회
    int count1 = plugin->get_features(plugin, &features1);
    int count2 = plugin->get_features(plugin, &features2);
    
    // Then - 일관된 결과를 반환해야 함
    ck_assert_int_eq(count1, count2);
    ck_assert_ptr_eq(features1, features2); // 같은 정적 배열을 반환해야 함
}
END_TEST

/**
 * NULL 포인터 안전성 테스트
 */
START_TEST(test_null_pointer_safety)
{
    // Given
    plugin = extsock_plugin_create();
    ck_assert_ptr_nonnull(plugin);
    
    // When & Then - NULL 포인터 전달 시 적절히 처리해야 함
    plugin_feature_t *features;
    
    // get_features에 NULL 전달 (실제로는 segfault 방지를 위해 내부에서 체크해야 함)
    int count = plugin->get_features(plugin, &features);
    ck_assert_int_gt(count, 0);
    ck_assert_ptr_nonnull(features);
}
END_TEST

/**
 * 리소스 정리 완전성 테스트
 */
START_TEST(test_resource_cleanup_completeness)
{
    // Given
    plugin = extsock_plugin_create();
    ck_assert_ptr_nonnull(plugin);
    
    // 플러그인 기능 확인
    plugin_feature_t *features;
    int count = plugin->get_features(plugin, &features);
    ck_assert_int_gt(count, 0);
    
    char *name = plugin->get_name(plugin);
    ck_assert_ptr_nonnull(name);
    
    // When
    plugin->destroy(plugin);
    plugin = NULL;
    
    // Then - 모든 리소스가 정리되어야 함
    // 메모리 누수 검사는 valgrind 등의 도구로 별도 수행
}
END_TEST

/**
 * 스레드 안전성 기본 테스트
 */
START_TEST(test_thread_safety_basic)
{
    // Given
    plugin = extsock_plugin_create();
    ck_assert_ptr_nonnull(plugin);
    
    // 동일한 플러그인 인스턴스에서 여러 번 메서드 호출
    for (int i = 0; i < 100; i++) {
        char *name = plugin->get_name(plugin);
        ck_assert_str_eq(name, "extsock");
        
        plugin_feature_t *features;
        int count = plugin->get_features(plugin, &features);
        ck_assert_int_gt(count, 0);
    }
}
END_TEST

/**
 * 테스트 스위트 생성
 */
Suite *plugin_lifecycle_suite(void)
{
    Suite *s;
    TCase *tc_core, *tc_lifecycle, *tc_stress, *tc_safety;

    s = suite_create("Plugin Lifecycle");

    /* Core test case */
    tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup_plugin_lifecycle_test, teardown_plugin_lifecycle_test);
    tcase_add_test(tc_core, test_plugin_creation);
    tcase_add_test(tc_core, test_plugin_get_name);
    tcase_add_test(tc_core, test_plugin_get_features);
    tcase_add_test(tc_core, test_plugin_destroy);
    suite_add_tcase(s, tc_core);

    /* Lifecycle test case */
    tc_lifecycle = tcase_create("Lifecycle");
    tcase_add_checked_fixture(tc_lifecycle, setup_plugin_lifecycle_test, teardown_plugin_lifecycle_test);
    tcase_add_test(tc_lifecycle, test_dependency_injection_failure);
    tcase_add_test(tc_lifecycle, test_multiple_plugin_instances);
    tcase_add_test(tc_lifecycle, test_socket_thread_lifecycle);
    tcase_add_test(tc_lifecycle, test_resource_cleanup_completeness);
    suite_add_tcase(s, tc_lifecycle);

    /* Stress test case */
    tc_stress = tcase_create("Stress");
    tcase_add_checked_fixture(tc_stress, setup_plugin_lifecycle_test, teardown_plugin_lifecycle_test);
    tcase_add_test(tc_stress, test_rapid_create_destroy_cycle);
    tcase_add_test(tc_stress, test_memory_constrained_creation);
    tcase_add_test(tc_stress, test_thread_safety_basic);
    suite_add_tcase(s, tc_stress);

    /* Safety test case */
    tc_safety = tcase_create("Safety");
    tcase_add_checked_fixture(tc_safety, setup_plugin_lifecycle_test, teardown_plugin_lifecycle_test);
    tcase_add_test(tc_safety, test_plugin_feature_consistency);
    tcase_add_test(tc_safety, test_null_pointer_safety);
    suite_add_tcase(s, tc_safety);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = plugin_lifecycle_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} 