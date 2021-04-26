/* Copyright (C) 2015-2021, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */
#if defined(Darwin) || (defined(__linux__) && defined(WAZUH_UNIT_TESTING))
#include "oslog_stream.h"

// Remove STATIC qualifier from tests
#ifdef WAZUH_UNIT_TESTING
#define STATIC
#else
#define STATIC static
#endif

/**
 * @brief Validates whether the predicate is valid or not
 * @param predicate Contains the `log stream`'s predicate filter to be used as a string
 * @return true if valid, otherwise false
 */
STATIC bool w_logcollector_validate_oslog_stream_predicate(char * predicate) {

    // todo : improve this function
    if (strlen(predicate) > 0) {
        return true;
    }
    return false;
}

/**
 * @brief Executes the `log stream` command with its arguments and sets to non-blocking the output pipe
 * @param predicate Contains the `log stream`'s predicate filter to be used as a string
 * @param level Contains, or not, the `log stream`'s level filter to be used as a string (default/info/debug)
 * @param type Contains the `log stream`'s type filters to be used (as combined bit flags)
 * @return A pointer to a fulfilled wfd_t structure, on success, or NULL
 */
STATIC char ** w_create_oslog_stream_array(char * predicate, char * level, int type) {

    char ** oslog_array = NULL;
    size_t oslog_array_idx = 0;

    os_calloc(MAX_LOG_CMD_ARGS + 1, sizeof(char *), oslog_array);

    // Adding `log` and `stream` to the array
    w_strdup(LOG_CMD_STR, oslog_array[oslog_array_idx++]);
    w_strdup(LOG_STREAM_OPT_STR, oslog_array[oslog_array_idx++]);

    // Adding the style lines to the array (`--style syslog`)
    w_strdup(STYLE_OPT_STR, oslog_array[oslog_array_idx++]);
    w_strdup(SYSLOG_STR, oslog_array[oslog_array_idx++]);

    // Log Stream's Type section (`--type`)
    if (type != 0) {
        if (type & OSLOG_TYPE_ACTIVITY) {
            w_strdup(TYPE_OPT_STR, oslog_array[oslog_array_idx++]);
            w_strdup(OSLOG_TYPE_ACTIVITY_STR, oslog_array[oslog_array_idx++]);
        }
        if (type & OSLOG_TYPE_LOG) {
            w_strdup(TYPE_OPT_STR, oslog_array[oslog_array_idx++]);
            w_strdup(OSLOG_TYPE_LOG_STR, oslog_array[oslog_array_idx++]);
        }
        if (type & OSLOG_TYPE_TRACE) {
            w_strdup(TYPE_OPT_STR, oslog_array[oslog_array_idx++]);
            w_strdup(OSLOG_TYPE_TRACE_STR, oslog_array[oslog_array_idx++]);
        }
    }

    // Log Stream's Level section  (`--level`)
    if (level != NULL) {
        w_strdup(LEVEL_OPT_STR, oslog_array[oslog_array_idx++]);
        w_strdup(level, oslog_array[oslog_array_idx++]);
    }

    // Log Stream's Predicate section
    if (predicate != NULL) {
        const bool is_predicate_valid = w_logcollector_validate_oslog_stream_predicate(predicate);
        if (is_predicate_valid) {
            w_strdup(PREDICATE_OPT_STR, oslog_array[oslog_array_idx++]);

            w_strdup(predicate, oslog_array[oslog_array_idx++]);
        }
    }

    return oslog_array;
}

/**
 * @brief Executes the `log stream` command with its arguments and sets to non-blocking the output pipe
 * @param oslog_array Contains the arguments of the command to be executed
 * @param flags Are the flags to be used along with wpopenv()
 * @return A pointer to a fulfilled wfd_t structure, on success, or NULL
 */
STATIC wfd_t * w_logcollector_exec_oslog_stream(char ** oslog_array, u_int32_t flags) {

    int oslog_fd = -1;
    int oslog_fd_flags = 0;
    wfd_t * oslog_wfd = wpopenv(*oslog_array, oslog_array, flags);

    if (oslog_wfd == NULL) {
        merror(WPOPENV_ERROR, strerror(errno), errno);
    } else {
        // The file descriptor, from which the output of `log stream` will be read, is set to non-blocking
        oslog_fd = fileno(oslog_wfd->file);                 // Gets the file descriptor from a file pointer

        if (oslog_fd <= 0) {
            merror(FP_TO_FD_ERROR, strerror(errno), errno);
            wpclose(oslog_wfd);
            oslog_wfd = NULL;
        } else {
            oslog_fd_flags = fcntl(oslog_fd, F_GETFL, 0);   // Gets current flags

            if (oslog_fd_flags < 0) {
                merror(GET_FLAGS_ERROR, strerror(errno), errno);
                wpclose(oslog_wfd);
                oslog_wfd = NULL;
            } else {
                oslog_fd_flags |= O_NONBLOCK;               // Adds the NON-BLOCKING flag to current flags
                const int set_flags_retval = fcntl(oslog_fd, F_SETFL, oslog_fd_flags);  // Sets the new Flags

                if (set_flags_retval < 0) {
                    merror(SET_FLAGS_ERROR, strerror(errno), errno);
                    wpclose(oslog_wfd);
                    oslog_wfd = NULL;
                }
            }
        }
    }

    return oslog_wfd;
}

void w_logcollector_create_oslog_env(logreader * current) {

    char ** log_stream_array = NULL;

    log_stream_array = w_create_oslog_stream_array(current->query, current->query_level, current->query_type);

    current->oslog->log_wfd = w_logcollector_exec_oslog_stream(log_stream_array, W_BIND_STDOUT | W_BIND_STDERR);

    if (current->oslog->log_wfd == NULL) {
        current->oslog->is_oslog_running = false;
    } else {
        current->oslog->is_oslog_running = true;
        minfo(LOG_STREAM_INFO, GET_LOG_STREAM_PARAMS(log_stream_array));
    }

    os_free(current->file);
    current->fp = NULL;

    free_strarray(log_stream_array);
}

#endif