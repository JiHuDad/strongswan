#include "vicisock_plugin.h"
#include <daemon.h>
#include <library.h>
#include <threading/thread.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <utils/debug.h>
#include <vici/libvici.h>
#include <settings/settings.h>
#include <cjson/cJSON.h>
#include <sa/ike_sa_manager.h>
#include <sa/ike_sa.h>
#include <sa/ikev2/tasks/ike_dpd.h>
#include <encoding/payloads/proposal_substructure.h>
#include <selectors/traffic_selector.h>
#include <crypto/proposal/proposal.h>
#include <utils/chunk.h>

#define VICISOCK_PATH "/tmp/strongswan_vicisock.sock"
#define VPN_CONF_PATH "/etc/strongswan/vpn.conf"

typedef struct private_vicisock_plugin_t private_vicisock_plugin_t;

struct private_vicisock_plugin_t {
    plugin_t public;
    int sock_fd;
    thread_t *thread;
    bool running;
    listener_t listener;
};

static void send_event_to_external(const char *json);
static void handle_load_all(int client);
static void handle_initiate(int client, cJSON *json);
static void handle_terminate(int client, cJSON *json);
static void handle_start_dpd(int client, const char *ike_sa_name);
static void handle_command(int client, const char *cmd, cJSON *json);
static void* vicisock_thread(void *arg);
static bool vicisock_child_updown(listener_t *listener, ike_sa_t *ike_sa, child_sa_t *child_sa, bool up);

// CHILD_SA UP/DOWN 이벤트 처리를 위한 리스너 인스턴스
static listener_t vicisock_listener = {
    .child_updown = vicisock_child_updown,
};

static bool is_list_key(const char *key) {
    const char *keys[] = {
        "local_addrs", "remote_addrs", "proposals", "esp_proposals", "ah_proposals",
        "local_ts", "remote_ts", "vips", "pools", "groups", "cert_policy"
    };
    for (size_t i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
        if (strcmp(keys[i], key) == 0) return TRUE;
    return FALSE;
}

static void add_list_key(vici_req_t *req, const char *key, const char *value) {
    vici_begin_list(req, (char*)key);
    char *tmp = strdup(value);
    char *token = strtok(tmp, ",");
    while (token) {
        vici_add_list_itemf(req, "%s", token);
        token = strtok(NULL, ",");
    }
    free(tmp);
    vici_end_list(req);
}

static bool add_key_values(vici_req_t *req, settings_t *cfg, const char *section) {
    enumerator_t *enumerator = cfg->create_key_value_enumerator(cfg, (char*)section);
    char *key, *value;
    while (enumerator->enumerate(enumerator, &key, &value)) {
        if (is_list_key(key)) {
            add_list_key(req, key, value);
        } else {
            vici_add_key_valuef(req, key, "%s", value);
        }
    }
    enumerator->destroy(enumerator);
    return TRUE;
}

static bool add_sections(vici_req_t *req, settings_t *cfg, const char *section) {
    enumerator_t *enumerator = cfg->create_section_enumerator(cfg, (char*)section);
    char *name, buf[256];
    while (enumerator->enumerate(enumerator, &name)) {
        vici_begin_section(req, name);
        snprintf(buf, sizeof(buf), "%s.%s", section, name);
        add_key_values(req, cfg, buf);
        add_sections(req, cfg, buf);
        vici_end_section(req);
    }
    enumerator->destroy(enumerator);
    return TRUE;
}

static bool load_all_conns_from_vpnconf(void) {
    settings_t *cfg = settings_create(VPN_CONF_PATH);
    if (!cfg) {
        DBG1(DBG_LIB, "vicisock: failed to load %s", VPN_CONF_PATH);
        return FALSE;
    }
    enumerator_t *conns = cfg->create_section_enumerator(cfg, "connections");
    char *conn_name;
    bool ok = TRUE;
    while (conns->enumerate(conns, &conn_name)) {
        char buf[256];
        snprintf(buf, sizeof(buf), "connections.%s", conn_name);
        vici_req_t *req = vici_begin("load-conn");
        vici_begin_section(req, conn_name);
        add_key_values(req, cfg, buf);
        add_sections(req, cfg, buf);
        vici_end_section(req);
        vici_res_t *res = vici_submit(req, NULL); // NULL: 내부 연결
        if (!res) {
            DBG1(DBG_LIB, "vicisock: vici_submit failed for conn %s", conn_name);
            ok = FALSE;
        } else {
            vici_free_res(res);
        }
        vici_free_req(req);
    }
    conns->destroy(conns);
    cfg->destroy(cfg);
    return ok;
}

static void handle_load_all(int client) {
    bool ok = load_all_conns_from_vpnconf();
    const char *msg = ok ? "{\"result\":\"ok\"}" : "{\"result\":\"fail\"}";
    ssize_t written = write(client, msg, strlen(msg));
    if (written < 0) {
        DBG1(DBG_LIB, "vicisock: write failed: %s", strerror(errno));
    }
}

static void handle_initiate(int client, cJSON *json) {
    cJSON *ikeitem = cJSON_GetObjectItem(json, "ike");
    if (!ikeitem || !cJSON_IsString(ikeitem)) return;
    vici_req_t *req = vici_begin("initiate");
    vici_add_key_value(req, "ike", ikeitem->valuestring, strlen(ikeitem->valuestring));
    vici_res_t *res = vici_submit(req, NULL);
    const char *msg = (res) ? "{\"result\":\"ok\"}" : "{\"result\":\"fail\"}";
    if (res) vici_free_res(res);
    vici_free_req(req);
    ssize_t written = write(client, msg, strlen(msg));
    if (written < 0) {
        DBG1(DBG_LIB, "vicisock: write failed: %s", strerror(errno));
    }
}

static void handle_terminate(int client, cJSON *json) {
    cJSON *ikeitem = cJSON_GetObjectItem(json, "ike");
    if (!ikeitem || !cJSON_IsString(ikeitem)) return;
    vici_req_t *req = vici_begin("terminate");
    vici_add_key_value(req, "ike", ikeitem->valuestring, strlen(ikeitem->valuestring));
    vici_res_t *res = vici_submit(req, NULL);
    const char *msg = (res) ? "{\"result\":\"ok\"}" : "{\"result\":\"fail\"}";
    if (res) vici_free_res(res);
    vici_free_req(req);
    ssize_t written = write(client, msg, strlen(msg));
    if (written < 0) {
        DBG1(DBG_LIB, "vicisock: write failed: %s", strerror(errno));
    }
}

static void handle_start_dpd(int client, const char *ike_sa_name) {
    ike_sa_t *ike_sa = charon->ike_sa_manager->checkout_by_name(
        charon->ike_sa_manager, (char*)ike_sa_name, ID_MATCH_PERFECT);
    const char *msg = NULL;
    if (!ike_sa) {
        DBG1(DBG_LIB, "vicisock: IKE_SA '%s' not found", ike_sa_name);
        msg = "{\"result\":\"fail\",\"reason\":\"not found\"}";
    } else {
        ike_dpd_t *dpd = ike_dpd_create(TRUE);
        ike_sa->queue_task(ike_sa, (task_t*)dpd);
        msg = "{\"result\":\"ok\"}";
    }
    ssize_t written = write(client, msg, strlen(msg));
    if (written < 0) {
        DBG1(DBG_LIB, "vicisock: write failed: %s", strerror(errno));
    }
}

static void handle_command(int client, const char *cmd, cJSON *json) {
    if (strcmp(cmd, "load-all") == 0) {
        handle_load_all(client);
    } else if (strcmp(cmd, "initiate") == 0) {
        handle_initiate(client, json);
    } else if (strcmp(cmd, "terminate") == 0) {
        handle_terminate(client, json);
    } else if (strcmp(cmd, "start-dpd") == 0) {
        cJSON *ikeitem = cJSON_GetObjectItem(json, "ike_sa");
        if (ikeitem && cJSON_IsString(ikeitem)) {
            handle_start_dpd(client, ikeitem->valuestring);
        }
    }
}

static void* vicisock_thread(void *arg)
{
    private_vicisock_plugin_t *this = arg;
    char buf[4096];
    DBG1(DBG_LIB, "vicisock: thread started");
    while (this->running) {
        int client = accept(this->sock_fd, NULL, NULL);
        if (client < 0) {
            DBG1(DBG_LIB, "vicisock: accept failed: %s", strerror(errno));
            continue;
        }
        DBG1(DBG_LIB, "vicisock: new client connected");
        ssize_t len = read(client, buf, sizeof(buf)-1);
        if (len > 0) {
            buf[len] = 0;
            DBG1(DBG_LIB, "vicisock: received command: %s", buf);
            cJSON *json = cJSON_Parse(buf);
            if (json) {
                cJSON *cmditem = cJSON_GetObjectItem(json, "command");
                if (cmditem && cJSON_IsString(cmditem)) {
                    DBG1(DBG_LIB, "vicisock: handling command: %s", cmditem->valuestring);
                    handle_command(client, cmditem->valuestring, json);
                } else {
                    DBG1(DBG_LIB, "vicisock: invalid command format");
                }
                cJSON_Delete(json);
            } else {
                DBG1(DBG_LIB, "vicisock: failed to parse JSON");
            }
        } else if (len == 0) {
            DBG1(DBG_LIB, "vicisock: client disconnected");
        } else {
            DBG1(DBG_LIB, "vicisock: read failed: %s", strerror(errno));
        }
        close(client);
    }
    DBG1(DBG_LIB, "vicisock: thread exiting");
    return NULL;
}

static void send_event_to_external(const char *json)
{
    DBG1(DBG_LIB, "vicisock: sending event: %s", json);
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        DBG1(DBG_LIB, "vicisock: socket creation failed: %s", strerror(errno));
        return;
    }
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, VICISOCK_PATH, sizeof(addr.sun_path)-1);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        ssize_t written = write(fd, json, strlen(json));
        if (written < 0) {
            DBG1(DBG_LIB, "vicisock: write failed: %s", strerror(errno));
        } else {
            DBG1(DBG_LIB, "vicisock: event sent successfully");
        }
    } else {
        DBG1(DBG_LIB, "vicisock: connect failed: %s", strerror(errno));
    }
    close(fd);
}

static bool vicisock_child_updown(listener_t *listener, ike_sa_t *ike_sa, child_sa_t *child_sa, bool up)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        DBG1(DBG_LIB, "vicisock: failed to create JSON object");
        return FALSE;
    }

    DBG1(DBG_LIB, "vicisock: processing %s event", up ? "tunnel-up" : "tunnel-down");
    cJSON_AddStringToObject(root, "event", up ? "tunnel-up" : "tunnel-down");
    cJSON_AddStringToObject(root, "ike", ike_sa ? ike_sa->get_name(ike_sa) : "");
    cJSON_AddStringToObject(root, "child", child_sa ? child_sa->get_name(child_sa) : "");

    if (child_sa) {
        // SA/SAD 정보
        cJSON *sa = cJSON_CreateObject();
        if (sa) {
            cJSON_AddNumberToObject(sa, "spi_in", child_sa->get_spi(child_sa, TRUE));
            cJSON_AddNumberToObject(sa, "spi_out", child_sa->get_spi(child_sa, FALSE));
            cJSON_AddNumberToObject(sa, "protocol", child_sa->get_protocol(child_sa));
            cJSON_AddNumberToObject(sa, "mode", child_sa->get_mode(child_sa));
            cJSON_AddNumberToObject(sa, "reqid", child_sa->get_reqid(child_sa));
            cJSON_AddNumberToObject(sa, "mark_in", child_sa->get_mark(child_sa, TRUE).value);
            cJSON_AddNumberToObject(sa, "mark_out", child_sa->get_mark(child_sa, FALSE).value);
            cJSON_AddNumberToObject(sa, "if_id_in", child_sa->get_if_id(child_sa, TRUE));
            cJSON_AddNumberToObject(sa, "if_id_out", child_sa->get_if_id(child_sa, FALSE));

            // 알고리즘/키 정보
            proposal_t *proposal = child_sa->get_proposal(child_sa);
            if (proposal) {
                uint16_t alg, keylen;
                // 암호화
                if (proposal->get_algorithm(proposal, ENCRYPTION_ALGORITHM, &alg, &keylen)) {
                    cJSON_AddNumberToObject(sa, "encr_alg", alg);
                    cJSON_AddNumberToObject(sa, "encr_keylen", keylen);
                }
                // 무결성
                if (proposal->get_algorithm(proposal, INTEGRITY_ALGORITHM, &alg, &keylen)) {
                    cJSON_AddNumberToObject(sa, "integ_alg", alg);
                }
            }
            cJSON_AddItemToObject(root, "sa", sa);
        }

        // SPD/정책 정보
        cJSON *spd = cJSON_CreateArray();
        if (spd) {
            for (int local = 0; local <= 1; local++) {
                enumerator_t *ts_enum = child_sa->create_ts_enumerator(child_sa, local);
                if (ts_enum) {
                    traffic_selector_t *ts;
                    while (ts_enum->enumerate(ts_enum, &ts)) {
                        cJSON *tsj = cJSON_CreateObject();
                        if (tsj) {
                            char buf[128];
                            snprintf(buf, sizeof(buf), "%R", ts);
                            cJSON_AddStringToObject(tsj, local ? "local_ts" : "remote_ts", buf);
                            cJSON_AddItemToArray(spd, tsj);
                        }
                    }
                    ts_enum->destroy(ts_enum);
                }
            }
            cJSON_AddItemToObject(root, "spd", spd);
        }
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        send_event_to_external(json_str);
        free(json_str);
    }
    cJSON_Delete(root);
    return TRUE;
}

static void vicisock_plugin_destroy(plugin_t *plugin)
{
    private_vicisock_plugin_t *this = (private_vicisock_plugin_t*)plugin;
    DBG1(DBG_LIB, "vicisock: plugin destruction started");
    
    this->running = FALSE;
    if (this->thread) {
        this->thread->cancel(this->thread);
        this->thread->join(this->thread);
    }
    if (this->sock_fd >= 0) {
        close(this->sock_fd);
        unlink(VICISOCK_PATH);
    }
    charon->bus->remove_listener(charon->bus, &vicisock_listener);
    DBG1(DBG_LIB, "vicisock: plugin destroyed");
    free(this);
}

static char* vicisock_plugin_get_name(plugin_t *plugin) {
    // Plugin name getter, rarely fails, no debug needed
    return (char*)"vicisock";
}

static int vicisock_plugin_get_features(plugin_t *plugin, plugin_feature_t **features)
{
    static plugin_feature_t f[] = {
        PLUGIN_PROVIDE(CUSTOM, "vicisock"),
    };
    *features = f;
    return countof(f);
}

plugin_t *vicisock_plugin_create()
{
    private_vicisock_plugin_t *this = calloc(1, sizeof(*this));
    if (!this) {
        DBG1(DBG_LIB, "vicisock: memory allocation failed");
        return NULL;
    }

    this->public.get_name = vicisock_plugin_get_name;

    this->public.get_features = vicisock_plugin_get_features;

    this->public.destroy = vicisock_plugin_destroy;

    this->running = TRUE;
    this->sock_fd = -1;

    this->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->sock_fd < 0) {
        DBG1(DBG_LIB, "vicisock: socket creation failed: %s", strerror(errno));
        free(this);
        return NULL;
    }
    DBG1(DBG_LIB, "vicisock: socket created successfully");

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, VICISOCK_PATH, sizeof(addr.sun_path)-1);
    unlink(VICISOCK_PATH);
    if (bind(this->sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        DBG1(DBG_LIB, "vicisock: bind failed: %s", strerror(errno));
        close(this->sock_fd);
        free(this);
        return NULL;
    }
    DBG1(DBG_LIB, "vicisock: socket bound to %s", VICISOCK_PATH);

    if (listen(this->sock_fd, 5) < 0) {
        DBG1(DBG_LIB, "vicisock: listen failed: %s", strerror(errno));
        close(this->sock_fd);
        free(this);
        return NULL;
    }
    DBG1(DBG_LIB, "vicisock: socket listening");

    this->thread = thread_create(vicisock_thread, this);
    if (!this->thread) {
        DBG1(DBG_LIB, "vicisock: thread creation failed");
        close(this->sock_fd);
        free(this);
        return NULL;
    }

    // 이벤트 리스너 등록 (tunnel up/down)
    charon->bus->add_listener(charon->bus, &vicisock_listener);
    DBG1(DBG_LIB, "vicisock: event listener registered");

    DBG1(DBG_LIB, "vicisock: plugin created successfully");
    return &this->public;
} 