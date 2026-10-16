/*
 * WinHub compatibility database: rules, backend choice, environment and command line
 *
 * Copyright 2026 WinHub contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "cJSON.h"
#include "compatdb.h"

void (*compat_warn)(const char *format, ...);

#define WARN_DB(...) do { if (compat_warn) compat_warn(__VA_ARGS__); } while (0)

static const char * const backend_names[COMPAT_BACKEND_COUNT] =
{
    "auto", "wined3d", "dxmt", "dxvk", "d3dmetal"
};

static const char * const hack_names[COMPAT_HACK_COUNT] =
{
    "graphics_backend", "dll_overrides", "env_vars", "append_cmd_line", "replace_exe_path"
};

int compat_backend_from_name(const char *name)
{
    int i;

    if (!name || !*name) return COMPAT_BACKEND_AUTO;
    for (i = 0; i < COMPAT_BACKEND_COUNT; i++)
        if (!strcmp(name, backend_names[i])) return i;
    return -1;
}

const char *compat_backend_name(enum compat_backend backend)
{
    return backend < COMPAT_BACKEND_COUNT ? backend_names[backend] : "<unknown>";
}

/* Paths compare with backslashes; rules may use either slash. */
static void to_backslashes(char *str)
{
    for (; *str; str++) if (*str == '/') *str = '\\';
}

void compat_strv_free(char **strv)
{
    char **p;

    if (!strv) return;
    for (p = strv; *p; p++) free(*p);
    free(strv);
}

static void free_hack(struct compat_hack *hack)
{
    unsigned int i;

    for (i = 0; i < hack->count; i++)
    {
        if (hack->names) free(hack->names[i]);
        if (hack->values) free(hack->values[i]);
    }
    free(hack->names);
    free(hack->values);
    free(hack->append);
    free(hack->required_args);
    free(hack->forbidden_args);
    free(hack->exe_path);
}

static void free_rule(struct compat_rule *rule)
{
    unsigned int i;

    for (i = 0; i < rule->hack_count; i++) free_hack(&rule->hacks[i]);
    free(rule->hacks);
    free(rule->desc);
    free(rule->path_suffix);
    free(rule->steam_game_id);
}

void compat_db_free(struct compat_db *db)
{
    unsigned int i;

    for (i = 0; i < db->count; i++) free_rule(&db->rules[i]);
    free(db->rules);
    memset(db, 0, sizeof(*db));
}

/* Optional string member: 0 if absent or a string, -1 if present with another type. */
static int get_optional_string(const cJSON *object, const char *key, char **out)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);

    *out = NULL;
    if (!item) return 0;
    if (!cJSON_IsString(item)) return -1;
    *out = strdup(item->valuestring);
    return 0;
}

static int get_optional_bool(const cJSON *object, const char *key, int *out)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);

    *out = 0;
    if (!item) return 0;
    if (!cJSON_IsBool(item)) return -1;
    *out = cJSON_IsTrue(item);
    return 0;
}

static int parse_hack(const cJSON *json, const char *desc, struct compat_hack *hack)
{
    const cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "type"), *item, *child;
    unsigned int i;

    memset(hack, 0, sizeof(*hack));
    if (!cJSON_IsObject(json) || !cJSON_IsString(type))
    {
        WARN_DB("rule %s: hack is not an object with a \"type\" string\n", desc);
        return -1;
    }

    for (i = 0; i < COMPAT_HACK_COUNT; i++)
        if (!strcmp(type->valuestring, hack_names[i])) break;
    if (i == COMPAT_HACK_COUNT)
    {
        WARN_DB("rule %s: unknown hack type %s\n", desc, type->valuestring);
        return -1;
    }
    hack->type = i;

    switch (hack->type)
    {
    case COMPAT_HACK_GRAPHICS_BACKEND:
        item = cJSON_GetObjectItemCaseSensitive(json, "backend");
        if (!cJSON_IsString(item) || compat_backend_from_name(item->valuestring) <= COMPAT_BACKEND_AUTO)
        {
            WARN_DB("rule %s: graphics_backend needs \"backend\": wined3d, dxmt, dxvk or d3dmetal\n", desc);
            return -1;
        }
        hack->backend = compat_backend_from_name(item->valuestring);
        if (get_optional_bool(json, "force", &hack->force))
        {
            WARN_DB("rule %s: graphics_backend \"force\" is not a boolean\n", desc);
            return -1;
        }
        return 0;

    case COMPAT_HACK_DLL_OVERRIDES:
        item = cJSON_GetObjectItemCaseSensitive(json, "overrides");
        if (!cJSON_IsArray(item) || !cJSON_GetArraySize(item))
        {
            WARN_DB("rule %s: dll_overrides needs a non-empty \"overrides\" array\n", desc);
            return -1;
        }
        hack->names = calloc(cJSON_GetArraySize(item), sizeof(char *));
        if (!hack->names) return -1;
        cJSON_ArrayForEach(child, item)
        {
            if (!cJSON_IsString(child) || !strchr(child->valuestring, '='))
            {
                WARN_DB("rule %s: dll override is not a \"dll=order\" string\n", desc);
                free_hack(hack);
                return -1;
            }
            hack->names[hack->count++] = strdup(child->valuestring);
        }
        return 0;

    case COMPAT_HACK_ENV_VARS:
        item = cJSON_GetObjectItemCaseSensitive(json, "vars");
        if (!cJSON_IsObject(item) || !cJSON_GetArraySize(item))
        {
            WARN_DB("rule %s: env_vars needs a non-empty \"vars\" object\n", desc);
            return -1;
        }
        hack->names = calloc(cJSON_GetArraySize(item), sizeof(char *));
        hack->values = calloc(cJSON_GetArraySize(item), sizeof(char *));
        if (!hack->names || !hack->values)
        {
            free_hack(hack);
            return -1;
        }
        cJSON_ArrayForEach(child, item)
        {
            if (!cJSON_IsString(child) || !child->string[0] || strchr(child->string, '='))
            {
                WARN_DB("rule %s: env var %s is not a valid name with a string value\n", desc, child->string);
                free_hack(hack);
                return -1;
            }
            hack->names[hack->count] = strdup(child->string);
            hack->values[hack->count++] = strdup(child->valuestring);
        }
        return 0;

    case COMPAT_HACK_APPEND_CMD_LINE:
        item = cJSON_GetObjectItemCaseSensitive(json, "append");
        if (!cJSON_IsString(item) || !item->valuestring[0])
        {
            WARN_DB("rule %s: append_cmd_line needs an \"append\" string\n", desc);
            return -1;
        }
        hack->append = strdup(item->valuestring);
        if (get_optional_string(json, "required_args", &hack->required_args) ||
            get_optional_string(json, "forbidden_args", &hack->forbidden_args) ||
            get_optional_bool(json, "insert", &hack->insert))
        {
            WARN_DB("rule %s: append_cmd_line has a key of the wrong type\n", desc);
            free_hack(hack);
            return -1;
        }
        return 0;

    case COMPAT_HACK_REPLACE_EXE_PATH:
        item = cJSON_GetObjectItemCaseSensitive(json, "path");
        if (!cJSON_IsString(item) || !item->valuestring[0])
        {
            WARN_DB("rule %s: replace_exe_path needs a \"path\" string\n", desc);
            return -1;
        }
        hack->exe_path = strdup(item->valuestring);
        to_backslashes(hack->exe_path);
        return 0;

    default:
        return -1;
    }
}

static int parse_rule(const cJSON *json, int index, struct compat_rule *rule)
{
    const cJSON *os, *match, *type, *value, *hacks, *child;
    char name[32];
    const char *desc;

    memset(rule, 0, sizeof(*rule));
    if (!cJSON_IsObject(json))
    {
        WARN_DB("rule %d is not an object\n", index);
        return -1;
    }
    if (get_optional_string(json, "desc", &rule->desc))
    {
        WARN_DB("rule %d: \"desc\" is not a string\n", index);
        return -1;
    }
    snprintf(name, sizeof(name), "#%d", index);
    desc = rule->desc ? rule->desc : name;

    os = cJSON_GetObjectItemCaseSensitive(json, "os");
    if (os && (!cJSON_IsString(os) || strcmp(os->valuestring, "macos")))
    {
        free_rule(rule);
        return -1;  /* for another system; not an error */
    }

    match = cJSON_GetObjectItemCaseSensitive(json, "match");
    type = cJSON_GetObjectItemCaseSensitive(match, "type");
    if (!cJSON_IsObject(match) || !cJSON_IsString(type))
    {
        WARN_DB("rule %s: \"match\" is not an object with a \"type\" string\n", desc);
        free_rule(rule);
        return -1;
    }
    if (!strcmp(type->valuestring, "path_suffix"))
    {
        value = cJSON_GetObjectItemCaseSensitive(match, "suffix");
        if (!cJSON_IsString(value) || !value->valuestring[0])
        {
            WARN_DB("rule %s: path_suffix match needs a \"suffix\" string\n", desc);
            free_rule(rule);
            return -1;
        }
        rule->path_suffix = strdup(value->valuestring);
        to_backslashes(rule->path_suffix);
    }
    else if (!strcmp(type->valuestring, "steam_game_id"))
    {
        value = cJSON_GetObjectItemCaseSensitive(match, "game_id");
        if (!cJSON_IsString(value) || !value->valuestring[0])
        {
            WARN_DB("rule %s: steam_game_id match needs a \"game_id\" string\n", desc);
            free_rule(rule);
            return -1;
        }
        rule->steam_game_id = strdup(value->valuestring);
    }
    else
    {
        WARN_DB("rule %s: unknown match type %s\n", desc, type->valuestring);
        free_rule(rule);
        return -1;
    }

    hacks = cJSON_GetObjectItemCaseSensitive(json, "hacks");
    if (!cJSON_IsArray(hacks))
    {
        WARN_DB("rule %s: \"hacks\" is not an array\n", desc);
        free_rule(rule);
        return -1;
    }
    rule->hacks = calloc(cJSON_GetArraySize(hacks) + 1, sizeof(*rule->hacks));
    if (!rule->hacks)
    {
        free_rule(rule);
        return -1;
    }
    cJSON_ArrayForEach(child, hacks)
        if (!parse_hack(child, desc, &rule->hacks[rule->hack_count])) rule->hack_count++;
    if (!rule->hack_count)
    {
        WARN_DB("rule %s has no valid hacks\n", desc);
        free_rule(rule);
        return -1;
    }
    return 0;
}

int compat_db_parse(const char *text, int require_timestamp, struct compat_db *db)
{
    cJSON *json = cJSON_Parse(text);
    const cJSON *version, *timestamp, *rules, *child;
    int index = 0;

    memset(db, 0, sizeof(*db));
    if (!cJSON_IsObject(json))
    {
        WARN_DB("not a JSON object\n");
        cJSON_Delete(json);
        return -1;
    }
    version = cJSON_GetObjectItemCaseSensitive(json, "version");
    if (!cJSON_IsNumber(version) || version->valueint != COMPAT_DB_VERSION)
    {
        WARN_DB("\"version\" is missing or not %d\n", COMPAT_DB_VERSION);
        cJSON_Delete(json);
        return -1;
    }
    timestamp = cJSON_GetObjectItemCaseSensitive(json, "timestamp");
    if (timestamp ? !cJSON_IsNumber(timestamp) : require_timestamp)
    {
        WARN_DB("\"timestamp\" is missing or not a number\n");
        cJSON_Delete(json);
        return -1;
    }
    if (timestamp) db->timestamp = timestamp->valuedouble;
    rules = cJSON_GetObjectItemCaseSensitive(json, "rules");
    if (!cJSON_IsArray(rules))
    {
        WARN_DB("\"rules\" is not an array\n");
        cJSON_Delete(json);
        return -1;
    }

    db->rules = calloc(cJSON_GetArraySize(rules) + 1, sizeof(*db->rules));
    if (!db->rules)
    {
        cJSON_Delete(json);
        return -1;
    }
    cJSON_ArrayForEach(child, rules)
        if (!parse_rule(child, index++, &db->rules[db->count])) db->count++;
    cJSON_Delete(json);
    return 0;
}

static char *read_file(const char *path)
{
    FILE *file;
    long size;
    char *text = NULL;

    if (!path || !(file = fopen(path, "rb"))) return NULL;
    if (!fseek(file, 0, SEEK_END) && (size = ftell(file)) >= 0 && !fseek(file, 0, SEEK_SET) &&
        (text = malloc(size + 1)))
    {
        if (fread(text, 1, size, file) == (size_t)size) text[size] = 0;
        else
        {
            free(text);
            text = NULL;
        }
    }
    fclose(file);
    return text;
}

static int load_file(const char *path, int require_timestamp, struct compat_db *db)
{
    char *text = read_file(path);
    int ret;

    memset(db, 0, sizeof(*db));
    if (!text) return -1;
    ret = compat_db_parse(text, require_timestamp, db);
    if (ret) WARN_DB("ignoring %s\n", path);
    free(text);
    return ret;
}

static void append_rules(struct compat_db *dst, struct compat_db *src)
{
    struct compat_rule *rules;

    if (!src->count) return;
    rules = realloc(dst->rules, (dst->count + src->count) * sizeof(*rules));
    if (!rules) return;
    memcpy(rules + dst->count, src->rules, src->count * sizeof(*rules));
    dst->rules = rules;
    dst->count += src->count;
    free(src->rules);  /* the rules themselves now belong to dst */
    memset(src, 0, sizeof(*src));
}

void compat_db_load(const char *builtin, const char *downloaded, const char *local, struct compat_db *db)
{
    struct compat_db builtin_db, downloaded_db, local_db;
    int have_builtin = !load_file(builtin, 1, &builtin_db);
    int have_downloaded = !load_file(downloaded, 1, &downloaded_db);

    memset(db, 0, sizeof(*db));
    if (!load_file(local, 0, &local_db)) append_rules(db, &local_db);
    compat_db_free(&local_db);

    if (have_downloaded && (!have_builtin || downloaded_db.timestamp > builtin_db.timestamp))
    {
        db->timestamp = downloaded_db.timestamp;
        append_rules(db, &downloaded_db);
    }
    else if (have_builtin)
    {
        if (have_downloaded) WARN_DB("%s is older than the built-in database; using the built-in one\n", downloaded);
        db->timestamp = builtin_db.timestamp;
        append_rules(db, &builtin_db);
    }
    compat_db_free(&builtin_db);
    compat_db_free(&downloaded_db);
}

static char ascii_lower(char c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

/* The path ends with the suffix, at a path component boundary, ignoring ASCII case. */
static int path_matches(const char *path, const char *suffix)
{
    size_t path_len = strlen(path), suffix_len = strlen(suffix);
    const char *tail;

    if (suffix_len > path_len) return 0;
    tail = path + path_len - suffix_len;
    if (tail > path && suffix[0] != '\\' && tail[-1] != '\\') return 0;
    for (; *tail; tail++, suffix++)
    {
        char c = *tail == '/' ? '\\' : *tail;
        if (ascii_lower(c) != ascii_lower(*suffix)) return 0;
    }
    return 1;
}

static int rule_matches(const struct compat_rule *rule, const char *image_path, const char *steam_game_id)
{
    if (rule->path_suffix) return image_path && path_matches(image_path, rule->path_suffix);
    if (rule->steam_game_id) return steam_game_id && !strcmp(steam_game_id, rule->steam_game_id);
    return 0;
}

void compat_db_find(const struct compat_db *db, const char *image_path, const char *steam_game_id,
                    const struct compat_hack *found[COMPAT_HACK_COUNT],
                    const struct compat_rule *found_rule[COMPAT_HACK_COUNT])
{
    unsigned int i, j;

    for (i = 0; i < COMPAT_HACK_COUNT; i++)
    {
        found[i] = NULL;
        if (found_rule) found_rule[i] = NULL;
    }
    for (i = 0; i < db->count; i++)
    {
        const struct compat_rule *rule = &db->rules[i];

        if (!rule_matches(rule, image_path, steam_game_id)) continue;
        for (j = 0; j < rule->hack_count; j++)
        {
            const struct compat_hack *hack = &rule->hacks[j];
            if (found[hack->type]) continue;
            found[hack->type] = hack;
            if (found_rule) found_rule[hack->type] = rule;
        }
    }
}

enum compat_backend compat_choose_backend(enum compat_backend user, const struct compat_hack *rule,
                                          int (*usable)(enum compat_backend backend, void *context),
                                          void *context)
{
    enum compat_backend candidates[8];
    unsigned int count = 0, i;

    if (rule && rule->force) candidates[count++] = rule->backend;
    if (user != COMPAT_BACKEND_AUTO) candidates[count++] = user;
    if (rule) candidates[count++] = rule->backend;
    candidates[count++] = COMPAT_BACKEND_D3DMETAL;
    candidates[count++] = COMPAT_BACKEND_DXMT;

    for (i = 0; i < count; i++)
    {
        if (candidates[i] == COMPAT_BACKEND_WINED3D) return COMPAT_BACKEND_WINED3D;
        if (usable(candidates[i], context)) return candidates[i];
    }
    return COMPAT_BACKEND_WINED3D;
}

static int env_name_matches(const char *entry, const char *name)
{
    size_t len = strlen(name);
    return !strncasecmp(entry, name, len) && entry[len] == '=';
}

const char *compat_env_get(const char *const *env, const char *name)
{
    unsigned int i;

    for (i = 0; env && env[i]; i++)
        if (env_name_matches(env[i], name)) return env[i] + strlen(name) + 1;
    return NULL;
}

char **compat_env_apply(const char *const *env, const struct compat_hack *hack)
{
    unsigned int count = 0, out = 0, i, j;
    char **result;

    while (env && env[count]) count++;
    if (!(result = calloc(count + hack->count + 1, sizeof(char *)))) return NULL;

    for (i = 0; i < count; i++)
    {
        for (j = 0; j < hack->count; j++)
            if (env_name_matches(env[i], hack->names[j])) break;
        if (j < hack->count) continue;
        if (!(result[out++] = strdup(env[i]))) goto failed;
    }
    for (j = 0; j < hack->count; j++)
    {
        size_t len = strlen(hack->names[j]) + strlen(hack->values[j]) + 2;
        if (!(result[out] = malloc(len))) goto failed;
        snprintf(result[out++], len, "%s=%s", hack->names[j], hack->values[j]);
    }
    return result;

failed:
    compat_strv_free(result);
    return NULL;
}

/* End of the program name at the start of a Windows command line. */
static size_t program_name_end(const char *cmdline)
{
    const char *p = cmdline;

    if (*p == '"')
    {
        p = strchr(p + 1, '"');
        return p ? (size_t)(p + 1 - cmdline) : strlen(cmdline);
    }
    while (*p && *p != ' ' && *p != '\t') p++;
    return p - cmdline;
}

char *compat_cmdline_apply(const char *cmdline, const struct compat_hack *hack)
{
    size_t len, split, append_len;
    char *result;

    if (!cmdline || !hack->append) return NULL;
    if (hack->required_args && !strcasestr(cmdline, hack->required_args)) return NULL;
    if (hack->forbidden_args && strcasestr(cmdline, hack->forbidden_args)) return NULL;
    if (strcasestr(cmdline, hack->append)) return NULL;

    len = strlen(cmdline);
    append_len = strlen(hack->append);
    split = hack->insert ? program_name_end(cmdline) : len;
    if (!(result = malloc(len + append_len + 2))) return NULL;
    memcpy(result, cmdline, split);
    result[split] = ' ';
    memcpy(result + split + 1, hack->append, append_len);
    strcpy(result + split + 1 + append_len, cmdline + split);
    return result;
}

char *compat_exe_replace_cmdline(const char *image_path, const char *cmdline, const struct compat_hack *hack)
{
    size_t path_len, rest_len, i;
    const char *rest = "";
    char *result;

    if (!hack->exe_path || !image_path) return NULL;
    path_len = strlen(hack->exe_path);
    if (strlen(image_path) == path_len)
    {
        for (i = 0; i < path_len; i++)
        {
            char a = image_path[i] == '/' ? '\\' : ascii_lower(image_path[i]);
            if (a != ascii_lower(hack->exe_path[i])) break;
        }
        if (i == path_len) return NULL;
    }

    if (cmdline) rest = cmdline + program_name_end(cmdline);
    rest_len = strlen(rest);
    if (!(result = malloc(path_len + rest_len + 3))) return NULL;
    result[0] = '"';
    memcpy(result + 1, hack->exe_path, path_len);
    result[path_len + 1] = '"';
    memcpy(result + path_len + 2, rest, rest_len + 1);
    return result;
}

static char *string_printf(const char *format, const char *a, const char *b)
{
    int len = snprintf(NULL, 0, format, a, b);
    char *str = len < 0 ? NULL : malloc((size_t)len + 1);

    if (str) snprintf(str, (size_t)len + 1, format, a, b);
    return str;
}

int compat_gst_paths(const char *root, const char *home, char **plugin_dir, char **registry)
{
    unsigned long long hash = 0xcbf29ce484222325ull;  /* FNV-1a */
    const unsigned char *p;
    char hex[17];

    *plugin_dir = *registry = NULL;
    if (!root) return 0;
    *plugin_dir = string_printf("%s%s", root, "/lib/gstreamer-1.0");
    if (home && *home)
    {
        for (p = (const unsigned char *)root; *p; p++) hash = (hash ^ *p) * 0x100000001b3ull;
        snprintf(hex, sizeof(hex), "%016llx", hash);
        *registry = string_printf("%s/Library/Caches/WinHub/gstreamer/registry-%s.x86_64.bin", home, hex);
    }
    return 1;
}
