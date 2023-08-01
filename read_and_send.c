#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <curl/curl.h>
#include <mysql/mysql.h>
#include <jansson.h>
#include <stdbool.h>

static volatile int running = 1;
static pid_t daemon_pid = 0; // Variable to store the daemon process ID

// Function to handle the stop signal
static void sig_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        running = 0;
    }
}

// Function to send data to the database using curl
static void send_data_to_db(const char* json_data) {
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl) {
        const char* url = "http://3.128.122.113/";
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(json_data));

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "Failed to send data to the database: %s\n", curl_easy_strerror(res));
        }
        // After performing the HTTP request
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        printf("HTTP Response Code: %ld\n", response_code);

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}
    // MySQL database connection parameters
    const char* db_host = "3.128.122.113";
    const char* db_user = "root";
    const char* db_password = "mySQL@dmin123";
    const char* db_name = "thermodb";

// Function to connect to the MySQL database and insert data
static int insert_temperature_and_state_data(float temperature, const char* state) {
    MYSQL* conn;
    // Initialize MySQL connection
    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed.\n");
        return 1;
    }

    // Connect to the MySQL server
    if (mysql_real_connect(conn, db_host, db_user, db_password, db_name, 0, NULL, 0) == NULL) {
        fprintf(stderr, "Failed to connect to database: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    // Insert data into the "temperature_data" table
    char insert_query[200];
    snprintf(insert_query, sizeof(insert_query),
             "INSERT INTO temperature_data (temperature, state) VALUES (%f, '%s')",
             temperature, state);
    if (mysql_query(conn, insert_query) != 0) {
        fprintf(stderr, "Failed to insert data into the database: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    mysql_close(conn);
    return 0;
}

// Function to read the file and send data to the database
static void read_and_send() {
    FILE* temp_fp;
    FILE* state_fp;
    char temp_buffer[100];
    char state_buffer[10];
    float temperature;
    char heater_state[4]; // "on" or "off"

    while (running) {
        // Open the "temp" file in read mode
        temp_fp = fopen("/tmp/temp", "r");
        state_fp = fopen("/tmp/status", "r");
        if (temp_fp == NULL || state_fp == NULL) {
            fprintf(stderr, "Error opening files.\n");
            sleep(2); // Wait 2 seconds before trying again
            continue;
        }

        // Read the temperature data from the file
        if (fgets(temp_buffer, sizeof(temp_buffer), temp_fp) != NULL) {
            temperature = atof(temp_buffer);
            printf("Temperature read from file: %.2f\n", temperature);
        }

        // Read the heater state from the file
        if (fgets(state_buffer, sizeof(state_buffer), state_fp) != NULL) {
            if (strcmp(state_buffer, "on\n") == 0) {
                strcpy(heater_state, "on");
            } else {
                strcpy(heater_state, "off");
            }
            printf("Heater state read from file: %s\n", heater_state);
        }

        fclose(temp_fp);
        fclose(state_fp);

        // Insert data into the MySQL database
        int result = insert_temperature_and_state_data(temperature, heater_state);
        if (result != 0) {
            fprintf(stderr, "Failed to insert temperature and heater state data into the database.\n");
        }

        sleep(10); // Wait 10 seconds before reading the files again
    }
}

int main() {
    // Daemonize the process
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Failed to fork.\n");
        return 1;
    }

    if (pid > 0) {
        // Parent process, save the daemon process ID and exit
        daemon_pid = pid;
        return 0;
    }

    // Child process, continue

    // Create a new session for the child process
    pid_t sid = setsid();
    if (sid < 0) {
        fprintf(stderr, "Failed to create a new session.\n");
        return 1;
    }

    // Change the working directory to a safe place
    if (chdir("/") < 0) {
        fprintf(stderr, "Failed to change the working directory.\n");
        return 1;
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Handle signals
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // Start the main daemon process
    read_and_send();

    return 0;
}