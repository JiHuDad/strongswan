#include "extsock_plugin.h"
#include "cJSON.h"
#include <daemon.h>
#include <library.h>
#include <threading/thread.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <kernel/kernel_ipsec.h>
#include <kernel/kernel_listener.h>
#include <config/config.h>
#include <config/ike_cfg.h>
#include <config/child_cfg.h>
#include <sa/ike_sa_manager.h>
#include <sa/ike_sa.h>
#include <sa/tasks/dpd.h>
#include <utils/host.h>

#define SOCKET_PATH "/tmp/strongswan_extsock.sock"

typedef struct private_extsock_plugin_t private_extsock_plugin_t;

struct private_extsock_plugin_t {
    plugin_t public;
    int sock_fd;
    thread_t *thread;
    bool running;
    kernel_listener_t *listener;
};

static void* socket_thread(private_extsock_plugin_t *this);

static void handle_external_command(private_extsock_plugin_t *this, char *cmd)
{
    if (strncmp(cmd, "START_DPD ", 10) == 0) {
        start_dpd(cmd + 10);
    } else if (strncmp(cmd, "APPLY_CONFIG ", 13) == 0) {
        apply_ipsec_config(cmd + 13);
    }
    // 기타 명령 처리
}

static void* socket_thread(private_extsock_plugin_t *this)
{
    struct sockaddr_un addr;
    char buf[1024];

    this->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->sock_fd < 0) {
        DBG1(DBG_LIB, "Failed to create socket");
        return NULL;
    }
    unlink(SOCKET_PATH);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    if (bind(this->sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        DBG1(DBG_LIB, "Failed to bind socket");
        close(this->sock_fd);
        return NULL;
    }
    listen(this->sock_fd, 5);

    this->running = TRUE;
    while (this->running) {
        int client = accept(this->sock_fd, NULL, NULL);
        if (client < 0) continue;
        ssize_t len = read(client, buf, sizeof(buf)-1);
        if (len > 0) {
            buf[len] = '\0';
            handle_external_command(this, buf);
        }
        close(client);
    }
    return NULL;
}

static void send_event_to_external(const char *event)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        write(fd, event, strlen(event));
    }
    close(fd);
}

static void send_sad_event(const char *event, kernel_ipsec_sa_id_t *id, kernel_ipsec_sa_t *sa)
{
    char buf[512];
    char src[64], dst[64];

    host_t *src_host = host_create_from_sockaddr((struct sockaddr*)&id->src);
    host_t *dst_host = host_create_from_sockaddr((struct sockaddr*)&id->dst);
    snprintf(src, sizeof(src), "%H", src_host);
    snprintf(dst, sizeof(dst), "%H", dst_host);

    snprintf(buf, sizeof(buf),
        "{\"event\":\"%s\",\"spi\":%u,\"proto\":%u,\"src\":\"%s\",\"dst\":\"%s\"}",
        event, id->spi, id->proto, src, dst);

    DESTROY_IF(src_host);
    DESTROY_IF(dst_host);

    send_event_to_external(buf);
}

static void send_spd_event(const char *event, kernel_ipsec_policy_id_t *id, kernel_ipsec_policy_t *policy)
{
    char buf[512];
    char src[64], dst[64];

    host_t *src_host = host_create_from_sockaddr((struct sockaddr*)&id->src);
    host_t *dst_host = host_create_from_sockaddr((struct sockaddr*)&id->dst);
    snprintf(src, sizeof(src), "%H", src_host);
    snprintf(dst, sizeof(dst), "%H", dst_host);

    const char *dir = (id->dir == POLICY_IN) ? "in" :
                      (id->dir == POLICY_OUT) ? "out" : "fwd";

    snprintf(buf, sizeof(buf),
        "{\"event\":\"%s\",\"id\":%u,\"proto\":%u,\"src\":\"%s\",\"dst\":\"%s\",\"dir\":\"%s\"}",
        event, id->reqid, id->proto, src, dst, dir);

    DESTROY_IF(src_host);
    DESTROY_IF(dst_host);

    send_event_to_external(buf);
}

static bool sad_add(kernel_listener_t *listener, kernel_ipsec_sa_id_t *id, kernel_ipsec_sa_t *sa)
{
    send_sad_event("SAD_ADD", id, sa);
    return TRUE;
}

static bool sad_delete(kernel_listener_t *listener, kernel_ipsec_sa_id_t *id)
{
    send_sad_event("SAD_DELETE", id, NULL);
    return TRUE;
}

static bool spd_add(kernel_listener_t *listener, kernel_ipsec_policy_id_t *id, kernel_ipsec_policy_t *policy)
{
    send_spd_event("SPD_ADD", id, policy);
    return TRUE;
}

static bool spd_delete(kernel_listener_t *listener, kernel_ipsec_policy_id_t *id)
{
    send_spd_event("SPD_DELETE", id, NULL);
    return TRUE;
}

static void register_kernel_listener(private_extsock_plugin_t *this)
{
    this->listener = kernel_listener_create(
        sad_add, sad_delete, spd_add, spd_delete,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    );
    charon->kernel->add_listener(charon->kernel, this->listener);
}

static void unregister_kernel_listener(private_extsock_plugin_t *this)
{
    if (this->listener) {
        charon->kernel->remove_listener(charon->kernel, this->listener);
        this->listener->destroy(this->listener);
        this->listener = NULL;
    }
}

static void extsock_plugin_destroy(private_extsock_plugin_t *this)
{
    this->running = FALSE;
    if (this->thread) {
        this->thread->cancel(this->thread);
        this->thread->join(this->thread);
        this->thread->destroy(this->thread);
    }
    if (this->sock_fd >= 0) {
        close(this->sock_fd);
        unlink(SOCKET_PATH);
    }
    free(this);
}

static void apply_ipsec_config(const char *json)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        DBG1(DBG_LIB, "Failed to parse JSON for IPsec config");
        return;
    }

    const char *conn_name = cJSON_GetObjectItem(root, "name")->valuestring;
    const char *local = cJSON_GetObjectItem(root, "local")->valuestring;
    const char *remote = cJSON_GetObjectItem(root, "remote")->valuestring;
    const char *psk = cJSON_GetObjectItem(root, "psk")->valuestring;

    ike_cfg_create_t ike = {
        .version = IKEV2,
        .local = local,
        .local_port = 500,
        .remote = remote,
        .remote_port = 500,
        .no_certreq = TRUE,
    };
    ike_cfg_t *ike_cfg = ike_cfg_create(&ike);
    ike_cfg->add_proposal(ike_cfg, proposal_create_default(PROTO_IKE));
    ike_cfg->add_proposal(ike_cfg, proposal_create_default_aead(PROTO_IKE));

    peer_cfg_create_t peer = {
        .cert_policy = CERT_NEVER_SEND,
        .unique = UNIQUE_REPLACE,
        .keyingtries = 1,
        .rekey_time = 3600,
        .jitter_time = 300,
        .over_time = 300,
        .dpd = 30,
    };
    peer_cfg_t *peer_cfg = peer_cfg_create(conn_name, ike_cfg, &peer);

    auth_cfg_t *auth = auth_cfg_create();
    auth->add(auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PSK);
    auth->add(auth, AUTH_RULE_IDENTITY, identification_create_from_string(local));
    peer_cfg->add_auth_cfg(peer_cfg, auth, TRUE);

    auth = auth_cfg_create();
    auth->add(auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PSK);
    auth->add(auth, AUTH_RULE_IDENTITY, identification_create_from_string(remote));
    peer_cfg->add_auth_cfg(peer_cfg, auth, FALSE);

    cJSON *child = cJSON_GetObjectItem(root, "child");
    const char *child_name = cJSON_GetObjectItem(child, "name")->valuestring;
    const char *local_ts = cJSON_GetObjectItem(child, "local_ts")->valuestring;
    const char *remote_ts = cJSON_GetObjectItem(child, "remote_ts")->valuestring;

    child_cfg_create_t child_cfg_data = {
        .mode = MODE_TUNNEL,
        .lifetime = {
            .time = { .life = 3600, .rekey = 3000, .jitter = 300 }
        }
    };
    child_cfg_t *child_cfg = child_cfg_create(child_name, &child_cfg_data);
    child_cfg->add_proposal(child_cfg, proposal_create_default_aead(PROTO_ESP));
    child_cfg->add_proposal(child_cfg, proposal_create_default(PROTO_ESP));
    child_cfg->add_traffic_selector(child_cfg, TRUE, traffic_selector_create_from_cidr(local_ts, 0, 0, 65535));
    child_cfg->add_traffic_selector(child_cfg, FALSE, traffic_selector_create_from_cidr(remote_ts, 0, 0, 65535));
    peer_cfg->add_child_cfg(peer_cfg, child_cfg);

    charon->backends->add_peer_cfg(charon->backends, peer_cfg);

    shared_key_t *key = shared_key_create(SHARED_IKE, chunk_create((u_char*)psk, strlen(psk)));
    charon->credentials->add_shared(charon->credentials, key);

    DBG1(DBG_LIB, "IPsec config applied successfully: %s", conn_name);

    cJSON_Delete(root);
}

static void start_dpd(const char *ike_name)
{
    ike_sa_t *ike_sa = charon->ike_sa_manager->checkout_by_name(
        charon->ike_sa_manager, ike_name, IKEV2, NULL);

    if (ike_sa) {
        task_t *dpd_task = (task_t*)dpd_initiator_create();
        ike_sa->queue_task(ike_sa, dpd_task);
        charon->ike_sa_manager->checkin(charon->ike_sa_manager, ike_sa);
        DBG1(DBG_LIB, "DPD Task queued successfully: %s", ike_name);
    } else {
        DBG1(DBG_LIB, "Failed to find IKE_SA: %s", ike_name);
    }
}

plugin_t *extsock_plugin_create()
{
    private_extsock_plugin_t *this = calloc(1, sizeof(*this));
    this->public.destroy = (void*)extsock_plugin_destroy;
    this->thread = thread_create((thread_main_t)socket_thread, this);
    register_kernel_listener(this);
    return &this->public;
} 