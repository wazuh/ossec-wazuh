/* Copyright (C) 2015-2021, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#ifndef MACOS_LOG_H
#define MACOS_LOG_H

/* ******************  INCLUDES  ****************** */

#include "shared.h"
#include "config/localfile-config.h"

/* ******************  DEFINES  ****************** */

#define MACOS_LOG_NAME          "macos" ///< Name to be displayed in the localfile' statistics

#define LOG_CMD_STR             "/usr/bin/log"  ///< It is the name of the command used to collect macos' logs

#define LOG_STREAM_OPT_STR      "stream"        ///< "stream" is a mode in which the "log" command can be executed
#define LOG_SHOW_OPT_STR        "show"          ///< "show" is a mode in which the "log" command can be executed

#define STYLE_OPT_STR           "--style"       ///< This precedes the logs' output style to be used by "log stream"
#define SYSLOG_STR              "syslog"        ///< This is the style chosen to show the "log stream" output

#define PREDICATE_OPT_STR       "--predicate"   ///< This precedes the "query" filter to be used by "log stream"
#define TYPE_OPT_STR            "--type"        ///< This precedes a "type" filter to be used by "log stream"
#define LEVEL_OPT_STR           "--level"       ///< This precedes the "level" filter to be used by "log stream"

#define SHOW_INFO_OPT_STR       "--info"        ///< Option to acquire up to the intermediate macOS log level
#define SHOW_DEBUG_OPT_STR      "--debug"       ///< Option to acquire all the macOS log levels

#define SHOW_START_OPT_STR      "--start"       ///< This option precedes the starting date to be used by "log show"

#define SHOW_TYPE_ACTIVITY_STR  "eventType == activityCreateEvent "         \
                                "OR eventType == activityTransitionEvent "  \
                                "OR eventType == userActionEvent"
#define SHOW_TYPE_LOG_STR       "eventType == logEvent"
#define SHOW_TYPE_TRACE_STR     "eventType == traceEvent"
#define SHOW_OR_TYPE_LOG_STR    " OR eventType == logEvent"
#define SHOW_OR_TYPE_TRACE_STR  " OR eventType == traceEvent"

#define MAX_LOG_CMD_ARGS        14
#define MAX_LOG_STREAM_CMD_ARGS MAX_LOG_CMD_ARGS    ///< This value takes into account the largest case of use
#define MAX_LOG_SHOW_CMD_ARGS   MAX_LOG_CMD_ARGS    ///< This value takes into account the largest case of use

#define QUERY_AND_TYPE_PREDICATE    "( %s ) AND ( %s )"

/** This macro is used for logging the full "log stream" command with its arguments (see MAX_LOG_STREAM_CMD_ARGS) **/
#define MACOS_GET_LOG_PARAMS(x)     x[0]?x[0]:"", x[1]?x[1]:"", x[2]?x[2]:"", \
                                    x[3]?x[3]:"", x[4]?x[4]:"", x[5]?x[5]:"", \
                                    x[6]?x[6]:"", x[7]?x[7]:"", x[8]?x[8]:"", \
                                    x[9]?x[9]:"", x[10]?x[10]:"",   \
                                    x[11]?x[11]:"", x[12]?x[12]:"", \
                                    x[13]?x[13]:""

#define MACOS_LOG_SHOW_CHILD_EXITED      LOGCOLLECTOR_MACOS_LOG_CHILD_EXITED,"show"
#define MACOS_LOG_STREAM_CHILD_EXITED    LOGCOLLECTOR_MACOS_LOG_CHILD_EXITED,"stream"


///< macOS ULS milliseconds lenght i.e .123456
#define OS_LOGCOLLECTOR_TIMESTAMP_MS_LEN        7
///< macOS ULS timezone lenght i.e -0700
#define OS_LOGCOLLECTOR_TIMESTAMP_TZ_LEN        5
///< macOS ULS basic timestamp lenght i.e 2021-04-27 08:07:20
#define OS_LOGCOLLECTOR_TIMESTAMP_BASIC_LEN     19
///< macOS ULS short timestamp lenght i.e 2021-04-27 08:07:20-0700
#define OS_LOGCOLLECTOR_TIMESTAMP_SHORT_LEN     OS_LOGCOLLECTOR_TIMESTAMP_BASIC_LEN + OS_LOGCOLLECTOR_TIMESTAMP_TZ_LEN
///< macOS ULS full timestamp lenght i.e 2020-11-09 05:45:08.000000-0800
#define OS_LOGCOLLECTOR_TIMESTAMP_FULL_LEN      OS_LOGCOLLECTOR_TIMESTAMP_SHORT_LEN + OS_LOGCOLLECTOR_TIMESTAMP_MS_LEN
///< JSON fields for file_status related to macOS ULS
#define OS_LOGCOLLECTOR_JSON_MACOS      MACOS_LOG_NAME
#define OS_LOGCOLLECTOR_JSON_TIMESTAMP  "timestamp"
#define OS_LOGCOLLECTOR_JSON_SETTINGS   "settings"

/* ******************  DATATYPES  ****************** */

typedef struct {
    pthread_rwlock_t mutex;
    char timestamp[OS_LOGCOLLECTOR_TIMESTAMP_SHORT_LEN + 1];
    char * settings;
} w_macos_log_vault_t;

/* ******************  PROTOTYPES  ****************** */

/**
 * @brief Creates the environment for collecting logs on macOS Systems
 * @param current logreader structure with `log`'s input arguments and w_macos_log_config_t structure to be set
 */
void w_macos_create_log_env(logreader * current);

/**
 * @brief Set string containing the last recorded timestamp.
 * 
 * @param timestamp macOS ULS short timestamp
 */
void w_macos_set_last_log_timestamp(char * timestamp);

/**
 * @brief Set string containing the last macOS ULS settings used.
 * 
 * @param predicate macOS ULS settings
 */
void w_macos_set_log_settings(char * settings);

/**
 * @brief Get string containing the last recorded timestamp.
 * 
 * @return Allocated string containing last recorded timestamp. NULL otherwise 
 */
char * w_macos_get_last_log_timestamp(void);

/**
 * @brief Get string containing the last macOS ULS settings used.
 * 
 * @return Allocated string containing last macOS ULS settings used. NULL otherwise 
 */
char * w_macos_get_log_settings(void);

/**
 * @brief Get macos vault as JSON
 * 
 * @return cJSON* macos vault
 */
cJSON * w_macos_get_status_as_JSON(void);

/**
 * @brief Set macos vault from JSON
 * 
 * @param global_json JSON object containing macos vault information
 */
void w_macos_set_status_from_JSON(cJSON * global_json);

#endif /* MACOS_LOG_H */