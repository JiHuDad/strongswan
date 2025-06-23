/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock_json_parser extsock_json_parser
 * @{ @ingroup extsock
 */

#ifndef EXTSOCK_JSON_PARSER_H_
#define EXTSOCK_JSON_PARSER_H_

#include <cjson/cJSON.h>
#include "../../common/extsock_types.h"
#include "../../interfaces/extsock_config_repository.h"

/**
 * JSON 파싱 어댑터
 * JSON 형태의 설정을 strongSwan 객체로 변환
 */
struct extsock_json_parser_t {
    
    /**
     * IKE 설정 파싱
     *
     * @param this      인스턴스
     * @param ike_json  IKE 설정 JSON
     * @return          파싱된 IKE 설정, 실패 시 NULL
     */
    ike_cfg_t* (*parse_ike_config)(extsock_json_parser_t *this, cJSON *ike_json);
    
    /**
     * 인증 설정 파싱
     *
     * @param this          인스턴스
     * @param auth_json     인증 설정 JSON
     * @param is_local      로컬 인증 여부
     * @return              파싱된 인증 설정, 실패 시 NULL
     */
    auth_cfg_t* (*parse_auth_config)(extsock_json_parser_t *this, 
                                    cJSON *auth_json, bool is_local);
    
    /**
     * 제안(Proposals) 파싱
     *
     * @param this          인스턴스
     * @param json_array    제안 JSON 배열
     * @param proto         프로토콜 (IKE/ESP/AH)
     * @param is_ike        IKE 제안 여부
     * @return              제안 리스트, 실패 시 NULL
     */
    linked_list_t* (*parse_proposals)(extsock_json_parser_t *this,
                                     cJSON *json_array, 
                                     protocol_id_t proto, 
                                     bool is_ike);
    
    /**
     * 트래픽 셀렉터 파싱
     *
     * @param this          인스턴스
     * @param json_array    TS JSON 배열
     * @return              TS 리스트, 실패 시 NULL
     */
    linked_list_t* (*parse_traffic_selectors)(extsock_json_parser_t *this,
                                             cJSON *json_array);
    
    /**
     * Child SA 설정 파싱
     *
     * @param this              인스턴스
     * @param peer_cfg          부모 피어 설정
     * @param children_json     Child SA JSON 배열
     * @return                  성공 시 TRUE
     */
    bool (*parse_child_configs)(extsock_json_parser_t *this,
                               peer_cfg_t *peer_cfg,
                               cJSON *children_json);
    
    /**
     * 전체 설정 엔티티 파싱
     *
     * @param this          인스턴스
     * @param config_json   설정 JSON 문자열
     * @return              파싱된 설정 엔티티, 실패 시 NULL
     */
    extsock_config_entity_t* (*parse_config_entity)(extsock_json_parser_t *this,
                                                   const char *config_json);
    
    /**
     * 인스턴스 소멸
     */
    void (*destroy)(extsock_json_parser_t *this);
};

/**
 * JSON 파싱 어댑터 생성
 *
 * @return  JSON 파싱 어댑터 인스턴스
 */
extsock_json_parser_t *extsock_json_parser_create();

#endif /** EXTSOCK_JSON_PARSER_H_ @}*/ 