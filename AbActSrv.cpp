#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Link with Ws2_32.lib inside Visual Studio
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT 80
#define BUFFER_SIZE  8192

// Reward payload tuples
struct RewardTuple {
    const char* promo_code;
    const char* type_val;
    const char* item_val;
};

// Game-tier containers for rewards
struct GameRewards {
    const char* game_type;
    const RewardTuple* rewards;
    int rewards_count;
};

// Define the sub-arrays for each game type first
const RewardTuple RVMS2013AC_REWARDS[] = {
    {"SUPERSEEDXX", "RVMS2013AC", "rovio-ad-codes-2"}
};

const RewardTuple HSBR2012TS_REWARDS[] = {
    {"BONUSLEVELX", "HSBR2012",   "bonus"},
    {"PATHOFJEDIX", "HSBR2012",   "dagobah"},
    {"ONEFALCONXX", "HSBR2012",   "falcon"},
    {"BOBAFETTXXX", "HSBR2012",   "bobafett"}
};

const RewardTuple HSBR2013TS_REWARDS[] = {
    {"CREDITTIERA", "HSBR2013TS", "cred-tier1"},
    {"HASBROCODEA", "HSBR2013TS", "hasbro-toy-codes-10"},
    {"CREDITTIERB", "HSBR2013TS", "cred-tier2"},
    {"HASBROCODEB", "HSBR2013TS", "hasbro-toy-codes-11"}
};

// Master nested map variable
const GameRewards CHECK_KEY_REWARDS[] = {
    {"RVMS2013AC", RVMS2013AC_REWARDS, sizeof(RVMS2013AC_REWARDS) / sizeof(RewardTuple)},
    {"HSBR2012TS", HSBR2012TS_REWARDS, sizeof(HSBR2012TS_REWARDS) / sizeof(RewardTuple)},
    {"HSBR2013TS", HSBR2013TS_REWARDS, sizeof(HSBR2013TS_REWARDS) / sizeof(RewardTuple)}
};
const int GAME_REWARDS_COUNT = sizeof(CHECK_KEY_REWARDS) / sizeof(GameRewards);

bool lookup_reward(const char* type_param, const char* key_param, const char** out_type, const char** out_item) {
    if (!type_param || !key_param) return false;

    // Find the outer game group matching 'types='
    for (int i = 0; i < GAME_REWARDS_COUNT; i++) {
        if (strcmp(type_param, CHECK_KEY_REWARDS[i].game_type) == 0) {
            
            // Find the inner coupon entry matching 'key='
            for (int j = 0; j < CHECK_KEY_REWARDS[i].rewards_count; j++) {
                if (strcmp(key_param, CHECK_KEY_REWARDS[i].rewards[j].promo_code) == 0) {
                    
                    // Found absolute match! Export values out
                    *out_type = CHECK_KEY_REWARDS[i].rewards[j].type_val;
                    *out_item = CHECK_KEY_REWARDS[i].rewards[j].item_val;
                    return true; 
                }
            }
        }
    }
    return false; // Key/Type didn't match anything
}

// Helper function to extract query parameters from the HTTP request line
void get_query_param(const char* request, const char* param_name, char* output, size_t max_len) {
    output[0] = '\0';
    if (!request || !param_name) return;

    // Look for the parameter name (e.g., "types=")
    const char* start = strstr(request, param_name);
    if (!start) return;

    start += strlen(param_name); // Move past the parameter key

    // Find the end of the parameter value (delimited by '&' or ' ' or '\r' or '\n')
    size_t i = 0;
    while (start[i] != '\0' && start[i] != '&' && start[i] != ' ' && start[i] != '\r' && start[i] != '\n' && i < (max_len - 1)) {
        output[i] = start[i];
        i++;
    }
    output[i] = '\0';
}

void handle_client(SOCKET client_socket) {
    char recv_buf[BUFFER_SIZE];
    int recv_len = recv(client_socket, recv_buf, BUFFER_SIZE - 1, 0);
    
    if (recv_len <= 0) {
        closesocket(client_socket);
        return;
    }

    recv_buf[recv_len] = '\0'; // Null-terminate incoming raw data

    // Log incoming traffic metadata to console
    printf("\n--- Got new request! ---\n");
    
    // Extract common parameters from the raw HTTP Request
    char types_val[256] = {0};
    char key_val[256] = {0};
    char udid_val[256] = {0};

    get_query_param(recv_buf, "types=", types_val, sizeof(types_val));
    get_query_param(recv_buf, "key=", key_val, sizeof(key_val));
    
    // Look for udid with fallback to uid
    get_query_param(recv_buf, "udid=", udid_val, sizeof(udid_val));
    if (strlen(udid_val) == 0) {
        get_query_param(recv_buf, "uid=", udid_val, sizeof(udid_val));
    }

    printf("Key:   %s\n", key_val);
    printf("Types: %s\n", types_val);
    printf("UDID:  %s\n", udid_val);

    char payload[512] = {0};
    bool handled = false;

    // Route 1: Core activation paths (consumeKey)
    if (strstr(recv_buf, "GET /consumeKey/") != NULL || strstr(recv_buf, "GET /drm/consumeKey/") != NULL) {
        _snprintf_s(payload, sizeof(payload), _TRUNCATE, "status=1&type=%s", types_val);
        handled = true;
    } 
    // Route 2: Coupon / Key validation paths (checkKey)
    else if (strstr(recv_buf, "GET /checkKey/") != NULL || strstr(recv_buf, "GET /drm/checkKey/") != NULL) {
        const char* out_type = NULL;
        const char* out_item = NULL;

        // Perform the nested hierarchy verification check
        if (lookup_reward(types_val, key_val, &out_type, &out_item)) {
            _snprintf_s(payload, sizeof(payload), _TRUNCATE, 
                "status=1&type=%s&group=%s", 
                out_type, out_item);
        } 
        else {
            // Unrecognized keys result in a generic failed registration envelope
            _snprintf_s(payload, sizeof(payload), _TRUNCATE, 
                "status=0&msg=invalid-key");
        }
        
        handled = true;
    }

    // Process HTTP response packaging
    if (handled) {
        char http_response[1024];
        _snprintf_s(http_response, sizeof(http_response), _TRUNCATE,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            (int)strlen(payload), payload);

        // Flush back to game client
        send(client_socket, http_response, (int)strlen(http_response), 0);
        printf("Returning: %s (200 OK)\n", payload);
    } 
    else {
        // Fallback response for unhandled endpoints
        const char* not_found = 
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
        send(client_socket, not_found, (int)strlen(not_found), 0);
        printf("Returning 404 Not Found\n");
    }

    // Gracefully shut down socket connection
    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;
    SOCKET listen_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    struct sockaddr_in server_addr;

    printf("Angry Birds PC Activation Server\n");
	printf("Classic / Rio / Seasons / Space / Star Wars / Star Wars II & Bad Piggies\n");
    printf("Listening on port %d...\n\n", DEFAULT_PORT);

    // Initialize WinSock v2.2 subsystem
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }

    // Allocate TCP socket
    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        printf("Socket creation failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set up local loopback listening bounds
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Only bind locally
    server_addr.sin_port = htons(DEFAULT_PORT);

    // Bind to Localhost Port 80
    if (bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        printf("Ensure no other app is using Port 80, and run as Admin!\n");
        closesocket(listen_socket);
        WSACleanup();
        system("pause");
        return 1;
    }

    // Begin listening for clients
    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    // Server loop execution
    while (TRUE) {
        client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket != INVALID_SOCKET) {
            handle_client(client_socket);
        }
    }

    // Cleanup resources (Unreachable in infinite loop)
    closesocket(listen_socket);
    WSACleanup();
    return 0;
}
