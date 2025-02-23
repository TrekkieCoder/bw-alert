#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>

#define RX_PATH "/sys/class/net/%s/statistics/rx_bytes"
#define TX_PATH "/sys/class/net/%s/statistics/tx_bytes"

// Function to get received bytes from the network interface
long long get_rx_bytes(const char *interface) {
    char path[256];
    FILE *file;
    long long bytes = 0;

    snprintf(path, sizeof(path), RX_PATH, interface);
    file = fopen(path, "r");
    if (file) {
        fscanf(file, "%lld", &bytes);
        fclose(file);
    } else {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    return bytes;
}

// Function to get transmitted bytes from the network interface
long long get_tx_bytes(const char *interface) {
    char path[256];
    FILE *file;
    long long bytes = 0;

    snprintf(path, sizeof(path), TX_PATH, interface);
    file = fopen(path, "r");
    if (file) {
        fscanf(file, "%lld", &bytes);
        fclose(file);
    } else {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    return bytes;
}

// Function to send an HTTP POST request to an IP address
void send_alert(const char *ip_address, const char *host_address, int state) {
    CURL *curl;
    CURLcode res;
    char url[256];
    char json_data[512];

    snprintf(url, sizeof(url), "http://%s:11111/netlox/v1/config/endpointhoststate", ip_address);

    // Create JSON payload dynamically with the host address
    snprintf(json_data, sizeof(json_data),
             "{ \"hostName\": \"%s\", \"state\": \"%s\" }", host_address, state ? "red":"green");

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "accept: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set timeout values
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);  // 2 seconds connection timeout
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);        // 5 seconds total timeout

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Curl request failed for %s: %s\n", ip_address, curl_easy_strerror(res));
        } else {
            printf("Alert sent to %s (Host: %s)\n", ip_address, host_address);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();
}

// Function to monitor bandwidth and send alerts if threshold is exceeded
void monitor_bandwidth(const char *interface, int interval, double rxthr, double txthr, char *ip_list, const char *host_address) {
    long long rx1, rx2;
    long long tx1, tx2;
    double rx_mbps;
    double tx_mbps;
    int health_ok = 1;
    int bad_retries = 0;
    int good_retries = 0;

    // Split comma-separated IPs into an array
    char *token;
    char *ip_addresses[100]; // Max 100 IPs
    int ip_count = 0;

    token = strtok(ip_list, ",");
    while (token != NULL) {
        ip_addresses[ip_count++] = token;
        token = strtok(NULL, ",");
    }

    while (1) {
        rx1 = get_rx_bytes(interface);
        tx1 = get_tx_bytes(interface);
        sleep(interval);
        rx2 = get_rx_bytes(interface);
        tx2 = get_tx_bytes(interface);

        // Calculate bandwidth in Mbps
        rx_mbps = ((rx2 - rx1) * 8.0) / (interval * 1000000);
        tx_mbps = ((tx2 - tx1) * 8.0) / (interval * 1000000);

        printf("Interface: %s | RX Bandwidth: %.2f Mbps | TX Bandwidth: %.2f Mbps\n", interface, rx_mbps, tx_mbps);

        // If bandwidth exceeds threshold, send alerts
        if (rx_mbps > rxthr || tx_mbps > txthr) {
            good_retries = 0;
            if (health_ok == 1) {
              for (int i = 0; i < ip_count; i++) {
                  printf("Threshold exceeded! Sending alerts...(%s)\n", ip_addresses[i]);
                  send_alert(ip_addresses[i], host_address, 1);
              }
              bad_retries++;
            }
            if (bad_retries >= 2) {
              health_ok = 0;
              bad_retries = 0;
            }
        } else if (!health_ok) {
            bad_retries = 0;
            for (int i = 0; i < ip_count; i++) {
              printf("Threshold normal! Sending alerts...(%s)\n", ip_addresses[i]);
              send_alert(ip_addresses[i], host_address, 0);
            }
            good_retries++;
            if (good_retries >= 2) {
              health_ok = 1;
              good_retries = 0;
            }
        }

        printf("--------------------------------------\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 7) {
        printf("Usage: %s <interface> <interval_seconds> <rx_threshold_mbps> <tx_threshold_mbps> <ip_list> <host_address>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *interface = argv[1];
    int interval = atoi(argv[2]);
    double rxthreshold = atof(argv[3]);
    double txthreshold = atof(argv[4]);
    char ip_list[1024];
    char host_address[256];

    strncpy(ip_list, argv[5], sizeof(ip_list) - 1);
    ip_list[sizeof(ip_list) - 1] = '\0';

    strncpy(host_address, argv[6], sizeof(host_address) - 1);
    host_address[sizeof(host_address) - 1] = '\0';

    monitor_bandwidth(interface, interval, rxthreshold, txthreshold, ip_list, host_address);

    return EXIT_SUCCESS;
}

