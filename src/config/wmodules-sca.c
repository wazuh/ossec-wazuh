/* Copyright (C) 2015-2019, Wazuh Inc.
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
*/

#include "wazuh_modules/wmodules.h"
#include "wazuh_modules/wm_sca.h"

#include <stdio.h>

static const char *XML_ENABLED = "enabled";
static const char *XML_SCAN_DAY = "day";
static const char *XML_WEEK_DAY = "wday";
static const char *XML_TIME = "time";
static const char *XML_INTERVAL = "interval";
static const char *XML_SCAN_ON_START= "scan_on_start";
static const char *XML_POLICIES = "policies";
static const char *XML_POLICY = "policy";
static const char *XML_SKIP_NFS = "skip_nfs";
static unsigned int profiles = 0;

static const size_t n_old_policies_hashes = 38;
static char * const old_policies_hashes[] = {
    "1803ba1239bef3a052d7866a43af0518000b2db9c54809ff1a3def2796394685",
    "18472a2d4c58df8e3c734456aeb970aad9a9e6a2b951bd3bbd0dcfb158cb7d1c",
    "1bd85961a1a03c0da4803a1237424dcab5fb15bfac4451d5fa098ff2c8eab3bd",
    "22b8f809c40042118762c5719e2e1fae39ec2891ccbb203d823b322e899fc1ae",
    "240c5d4991cfb8af53549f911840f34f11b5dacfd9862551cd310957de785aa0",
    "2527adc233514ee208a3716315a50944bc287d45dd70783db1b738a96b954a41",
    "310a00288036db576f27d225c433706fec7f5d2466a1d14c90ca4e83e127b033",
    "314687626d785733c07f9279fb7709432aa1fd1edce13f4e634dcfecff6fbde9",
    "396442ff7e5105958d7a2412c1cd65832b1bbee9d998570d49c083b7747a2516",
    "4c7d05c9501ea38910e20ae22b1670b4f778669bd488482b4a19d179da9556ea",
    "4f6d819cbd095124acf4b2081eb7822e8cf54db03b51b934c9baa448be71e68c",
    "5d2b2fcfb8f8946e7f9873dc9cb9f782551770ccfc5aed92a161fc2ee0b34f1e",
    "6cc1dd0bdb7e80c3f9db43979ea810296a89902ee07e1969d0601b8f12761385",
    "6e551fb7d1d094dbef36b9a49d1c30d8bb96d7dcb2103f9c90119868cd053699",
    "80601df8db93f800b5ad76a7b48bff5cfa44a9c08027a46985a10784768b84ef",
    "855f03da194a2d27557efbf281c48161bfca1ea49c5a8dec6e55e36e2688456f",
    "a476cef99b877eecc660db76c5f1c1ba73fdde1dd317cd49e04dbdd2d01fec1b",
    "a4ad3c3a1409e04217eabb884589717861b9389b68eba897093a5571a7671e2f",
    "aac8316c5532613800a78ebe92b74ce785f12f74c7358a3b99bac36a1fe98155",
    "ad5b2984fdb9dc1dffd71097cf7bb2d31a7f82fca845fd5c8cd4c5b36cf13098",
    "ae1502f6c8358909c5271258343fbb0b657b7a523512dfc88a321030e0713211",
    "bba7b4e84e093f825b979000858867b133fc1af4ca14d1c66cfa01af4885144e",
    "c1ad79c40ea9bd89ce859ea215c7502b050c36b612aae2d79ea9d8c055d818c1",
    "c4cf49ae248ddb5a3f0cd89fa659d64c0d6f90f9fdfcfc41defd8b0e916f1281",
    "e0d13d11c15f05582e27706c759ed1d261678ba1010b3d5d394fa73a6b49d0fc",
    "e51768c42f2dd182357b8674298e0f9fa0f8325c0a4f0d41055c68f675cb77fb",
    "f3973d0f9d5699d3b22b8cc5a1b135cd16e6ac4d5dd2a6aa2f44a4c2b324fbd9",
    "fcb5f2e73f5f9b62b5127970a88275ece7b515cdf6cfcb98548f5cdde5fbe0bf",
    "4d817b980d7802bcc32c3e713b5a60e10a96944e1ed226f55800676d4c0ae334",
    "73f7a114169a0c19c7489b8f5847a3da0a1041f946d5d71d534a475adf4da693",
    "7e194d3768067bbba7cb761a2ab4b557783373aa488486459ac181f85fc0416d",
    "8db06ce8c56fb7ed50255b5191e3835632b649aeb642c7948c4ac020f1311141",
    "961fe962fb0f3ad96d2d588ffd741f38263d8378d9ffc812ea3308c0e8227ac7",
    "aabcd560435331805a8d8572b90e9a4ad69ec49e918c14e14abf51b3953fe070",
    "baadc4a0f6ab539d535ae337319a34625ca9c2f82d5fc450cf8365b3d0cd88c9",
    "cc3cb14805d89166e93cfe2427182322e6f4cbfd7cdf6254c024bd3c96884d32",
    "e8e514d5686e22695f33c9afc6e1cfcc345b0cbfc4ee6fff1feabb3d1cec22ec",
    "fc68aec08668ceec29e009697e1958c60a6f70cbe2942740dc434880bd2a663c"
};

#undef minfo
#undef mwarn
#undef merror
#undef mdebug1
#undef mdebug2

#define minfo(msg, ...) _mtinfo(WM_SCA_LOGTAG, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define mwarn(msg, ...) _mtwarn(WM_SCA_LOGTAG, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define merror(msg, ...) _mterror(WM_SCA_LOGTAG, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define mdebug1(msg, ...) _mtdebug1(WM_SCA_LOGTAG, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define mdebug2(msg, ...) _mtdebug2(WM_SCA_LOGTAG, __FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)

static int is_policy_old (char * const hash_array[], size_t hash_array_len, const char * const policy_filename);

static int is_policy_old (char * const hash_array[], size_t hash_array_len, const char * const policy_filename)
{
    char full_path[PATH_MAX] = {0};
    #ifdef WIN32
    sprintf(full_path, "%s/%s", SECURITY_CONFIGURATION_ASSESSMENT_DIR_WIN, policy_filename);
    #else
    sprintf(full_path, "%s/%s", DEFAULTDIR SECURITY_CONFIGURATION_ASSESSMENT_DIR, policy_filename);
    #endif

    char *file_hash = wm_sca_hash_integrity_file(full_path);
    if (find_string_in_array(hash_array, hash_array_len, file_hash, 64)) {
        os_free(file_hash);
        return 1;
    }

    os_free(file_hash);
    return 0;
}

static short eval_bool(const char *str)
{
    return !str ? OS_INVALID : !strcmp(str, "yes") ? 1 : !strcmp(str, "no") ? 0 : OS_INVALID;
}

// Reading function
int wm_sca_read(const OS_XML *xml,xml_node **nodes, wmodule *module)
{
    unsigned int i;
    int month_interval = 0;
    wm_sca_t *sca;

    if(!module->data) {
        os_calloc(1, sizeof(wm_sca_t), sca);
        sca->enabled = 1;
        sca->scan_on_start = 1;
        sca->scan_wday = -1;
        sca->scan_day = 0;
        sca->scan_time = NULL;
        sca->skip_nfs = 1;
        sca->alert_msg = NULL;
        sca->queue = -1;
        sca->interval = WM_DEF_INTERVAL / 2;
        sca->profile = NULL;
        module->context = &WM_SCA_CONTEXT;
        module->tag = strdup(module->context->name);
        module->data = sca;
        profiles = 0;
    }

    sca = module->data;

    /* By default, load all every ruleset present */

    char ruleset_path[PATH_MAX] = {0};
    #ifdef WIN32
    sprintf(ruleset_path, "%s/", SECURITY_CONFIGURATION_ASSESSMENT_DIR_WIN);
    #else
    sprintf(ruleset_path, "%s/", DEFAULTDIR SECURITY_CONFIGURATION_ASSESSMENT_DIR);
    #endif

    DIR *ruleset_dir = opendir(ruleset_path);
    const int open_dir_errno = errno;
    if (ruleset_dir) {
        struct dirent *dir_entry;
        while ((dir_entry = readdir(ruleset_dir)) != NULL) {
            if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) {
                continue;
            }

            if (is_policy_old(old_policies_hashes, n_old_policies_hashes, dir_entry->d_name)) {
                minfo("Skipping outdated policy file '%s' (policy file removed)", dir_entry->d_name);
                continue;
            }

            const char * const file_extension = strrchr(dir_entry->d_name, '.');
            if (!file_extension || (strcmp(file_extension, ".yml") != 0 && strcmp(file_extension, ".yaml") != 0)) {
                continue;
            }

            minfo("Adding policy file '%s' by default.", dir_entry->d_name);

            os_realloc(sca->profile, (profiles + 2) * sizeof(wm_sca_profile_t *), sca->profile);
            wm_sca_profile_t *policy;
            os_calloc(1,sizeof(wm_sca_profile_t),policy);

            policy->enabled = 1;
            policy->policy_id = NULL;
            policy->remote = 0;
            os_strdup(dir_entry->d_name, policy->profile);
            sca->profile[profiles] = policy;
            sca->profile[profiles + 1] = NULL;
            profiles++;
        }

        closedir(ruleset_dir);
    } else {
        minfo("Could not open the default SCA ruleset folder '%s': %s", ruleset_path, strerror(open_dir_errno));
    }

    if(!sca->alert_msg) {
        /* We store up to 255 alerts */
        os_calloc(256, sizeof(char *), sca->alert_msg);
    }

    if (!nodes) {
        return 0;
    }

    for(i = 0; nodes[i]; i++)
    {
        if(!nodes[i]->element)
        {
            merror(XML_ELEMNULL);
            return OS_INVALID;
        }
        else if (!strcmp(nodes[i]->element, XML_ENABLED))
        {
            int enabled = eval_bool(nodes[i]->content);

            if(enabled == OS_INVALID){
                merror("Invalid content for tag '%s'", XML_ENABLED);
                return OS_INVALID;
            }

            sca->enabled = enabled;
        }
        else if (!strcmp(nodes[i]->element, XML_WEEK_DAY))
        {
            sca->scan_wday = w_validate_wday(nodes[i]->content);
            if (sca->scan_wday < 0 || sca->scan_wday > 6) {
                merror(XML_VALUEERR, nodes[i]->element, nodes[i]->content);
                return (OS_INVALID);
            }
        }
        else if (!strcmp(nodes[i]->element, XML_SCAN_DAY)) {
            if (!OS_StrIsNum(nodes[i]->content)) {
                merror(XML_VALUEERR, nodes[i]->element, nodes[i]->content);
                return (OS_INVALID);
            } else {
                sca->scan_day = atoi(nodes[i]->content);
                if (sca->scan_day < 1 || sca->scan_day > 31) {
                    merror(XML_VALUEERR, nodes[i]->element, nodes[i]->content);
                    return (OS_INVALID);
                }
            }
        }
        else if (!strcmp(nodes[i]->element, XML_TIME))
        {
            sca->scan_time = w_validate_time(nodes[i]->content);
            if (!sca->scan_time) {
                merror(XML_VALUEERR, nodes[i]->element, nodes[i]->content);
                return (OS_INVALID);
            }
        }
        else if (!strcmp(nodes[i]->element, XML_INTERVAL)) {
            char *endptr;
            sca->interval = strtoul(nodes[i]->content, &endptr, 0);

            if (sca->interval == 0 || sca->interval == UINT_MAX) {
                merror("Invalid interval value.");
                return OS_INVALID;
            }

            switch (*endptr) {
            case 'M':
                month_interval = 1;
                sca->interval *= 60; // We can`t calculate seconds of a month
                break;
            case 'w':
                sca->interval *= 604800;
                break;
            case 'd':
                sca->interval *= 86400;
                break;
            case 'h':
                sca->interval *= 3600;
                break;
            case 'm':
                sca->interval *= 60;
                break;
            case 's':
            case '\0':
                break;
            default:
                merror("Invalid interval value.");
                return OS_INVALID;
            }

            if (sca->interval < 60) {
                mwarn("Interval must be greater than 60 seconds. New interval value: 60s");
                sca->interval = 60;
            }
        }
        else if (!strcmp(nodes[i]->element, XML_SCAN_ON_START))
        {
            int scan_on_start = eval_bool(nodes[i]->content);

            if(scan_on_start == OS_INVALID)
            {
                merror("Invalid content for tag '%s'", XML_ENABLED);
                return OS_INVALID;
            }

            sca->scan_on_start = scan_on_start;
        }
        else if (!strcmp(nodes[i]->element, XML_POLICIES))
        {
            /* Get children */
            xml_node **children = NULL;
            if (children = OS_GetElementsbyNode(xml, nodes[i]), !children) {
                return OS_INVALID;
            }

            int  j;
            for (j = 0; children[j]; j++) {
                if (strcmp(children[j]->element, XML_POLICY) == 0) {
                    int enabled = 1;
                    int policy_found = 0;

                    if(children[j]->attributes && children[j]->values) {

                        if(strcmp(*children[j]->attributes,XML_ENABLED) == 0){
                            if(strcmp(*children[j]->values,"no") == 0){
                                enabled = 0;
                            }
                        }
                    }

                    if(strlen(children[j]->content) >= PATH_MAX) {
                        merror("Policy path is too long. Max path length is %d.", PATH_MAX);
                        OS_ClearNode(children);
                        return OS_INVALID;
                    } else if (strlen(children[j]->content) == 0) {
                        merror("Empty policy value.");
                        OS_ClearNode(children);
                        return OS_INVALID;
                    }

                    if(sca->profile) {
                        int i;
                        for(i = 0; sca->profile[i]; i++) {
                            if(!strcmp(sca->profile[i]->profile, children[j]->content)) {
                                sca->profile[i]->enabled = enabled;
                                if(!enabled) {
                                    minfo("Disabling policy '%s' by configuration.", children[j]->content);
                                }
                                policy_found = 1;
                                break;
                            }
                        }
                    }

                    if (is_policy_old(old_policies_hashes, n_old_policies_hashes, children[j]->content)) {
                        minfo("Skipping outdated policy file '%s' (policy file removed)", children[j]->content);
                        continue;
                    }

                    if(!policy_found) {
                        os_realloc(sca->profile, (profiles + 2) * sizeof(wm_sca_profile_t *), sca->profile);
                        wm_sca_profile_t *policy;
                        os_calloc(1,sizeof(wm_sca_profile_t),policy);
                        minfo("Adding policy file '%s'", children[j]->content);
                        policy->enabled = enabled;
                        policy->policy_id = NULL;
                        policy->remote = strstr(children[j]->content, "etc/shared/") != NULL;
                        os_strdup(children[j]->content, policy->profile);
                        sca->profile[profiles] = policy;
                        sca->profile[profiles + 1] = NULL;
                        profiles++;
                    }
                } else {
                    merror(XML_ELEMNULL);
                    OS_ClearNode(children);
                    return OS_INVALID;
                }
            }
            OS_ClearNode(children);
        }
        else if (!strcmp(nodes[i]->element, XML_SKIP_NFS))
        {
            int skip_nfs = eval_bool(nodes[i]->content);

            if(skip_nfs == OS_INVALID){
                merror("Invalid content for tag '%s'", XML_SKIP_NFS);
                return OS_INVALID;
            }

            sca->skip_nfs = skip_nfs;
        }
        else
        {
            mwarn("No such tag <%s>", nodes[i]->element);
        }
    }

    // Validate scheduled scan parameters and interval value

    if (sca->scan_day && (sca->scan_wday >= 0)) {
        merror("Options 'day' and 'wday' are not compatible.");
        return OS_INVALID;
    } else if (sca->scan_day) {
        if (!month_interval) {
            mwarn("Interval must be a multiple of one month. New interval value: 1M");
            sca->interval = 60; // 1 month
        }
        if (!sca->scan_time)
            sca->scan_time = strdup("00:00");
    } else if (sca->scan_wday >= 0) {
        if (w_validate_interval(sca->interval, 1) != 0) {
            sca->interval = 604800;  // 1 week
            mwarn("Interval must be a multiple of one week. New interval value: 1w");
        }
        if (sca->interval == 0)
            sca->interval = 604800;
        if (!sca->scan_time)
            sca->scan_time = strdup("00:00");
    } else if (sca->scan_time) {
        if (w_validate_interval(sca->interval, 0) != 0) {
            sca->interval = WM_DEF_INTERVAL;  // 1 day
            mwarn("Interval must be a multiple of one day. New interval value: 1d");
        }
    }

    return 0;
}
