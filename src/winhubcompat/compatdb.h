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

/* Plain C (libc and cJSON only) so it can be tested outside Wine. All strings are UTF-8.
 * The file format is described in docs/compatdb.md. */

#ifndef WINHUB_COMPATDB_H
#define WINHUB_COMPATDB_H

#include <stddef.h>

#define COMPAT_DB_VERSION 1

enum compat_backend
{
    COMPAT_BACKEND_AUTO,
    COMPAT_BACKEND_WINED3D,
    COMPAT_BACKEND_DXMT,
    COMPAT_BACKEND_DXVK,
    COMPAT_BACKEND_D3DMETAL,
    COMPAT_BACKEND_COUNT
};

enum compat_hack_type
{
    COMPAT_HACK_GRAPHICS_BACKEND,
    COMPAT_HACK_DLL_OVERRIDES,
    COMPAT_HACK_ENV_VARS,
    COMPAT_HACK_APPEND_CMD_LINE,
    COMPAT_HACK_COUNT
};

struct compat_hack
{
    enum compat_hack_type type;
    enum compat_backend backend; /* graphics_backend */
    int force;                   /* graphics_backend: also wins over the user's choice */
    unsigned int count;          /* dll_overrides, env_vars */
    char **names;                /* dll_overrides entries ("d3d11=n,b"); env_vars names */
    char **values;               /* env_vars values */
    char *append;                /* append_cmd_line */
    char *required_args;         /* append_cmd_line: only if the command line contains this */
    char *forbidden_args;        /* append_cmd_line: not if the command line contains this */
    int insert;                  /* append_cmd_line: right after the program name instead of at the end */
};

struct compat_rule
{
    char *desc;
    char *path_suffix;           /* matches the end of the program's Windows path */
    char *steam_game_id;         /* matches SteamGameId */
    unsigned int hack_count;
    struct compat_hack *hacks;
};

struct compat_db
{
    double timestamp;
    unsigned int count;
    struct compat_rule *rules;
};

/* Receives warnings about invalid files and rules; NULL drops them. */
extern void (*compat_warn)(const char *format, ...);

/* Parses one file's contents. Invalid rules are skipped with a warning; returns 0, or -1 if the
 * file itself is unusable. require_timestamp: the "timestamp" key must be present. */
int compat_db_parse(const char *json, int require_timestamp, struct compat_db *db);
void compat_db_free(struct compat_db *db);

/* Reads the rules in order of precedence: the user's local rules first, then the newer of the
 * built-in and the downloaded database (the built-in one on a tie). Missing files are fine;
 * any path may be NULL. */
void compat_db_load(const char *builtin, const char *downloaded, const char *local, struct compat_db *db);

/* For each hack type, the first hack of that type among the rules matching the program. */
void compat_db_find(const struct compat_db *db, const char *image_path, const char *steam_game_id,
                    const struct compat_hack *found[COMPAT_HACK_COUNT],
                    const struct compat_rule *found_rule[COMPAT_HACK_COUNT]);

/* "auto", "wined3d", "dxmt", "dxvk", "d3dmetal"; NULL or "" is auto; anything else is -1. */
int compat_backend_from_name(const char *name);
const char *compat_backend_name(enum compat_backend backend);

/* The backend to use: the first usable one among a forced rule, the user's choice, the rule, then
 * d3dmetal, dxmt and wined3d. wined3d is always taken as usable. */
enum compat_backend compat_choose_backend(enum compat_backend user, const struct compat_hack *rule,
                                          int (*usable)(enum compat_backend backend, void *context),
                                          void *context);

/* New NULL-terminated environment ("NAME=VALUE" strings) with the hack's variables set,
 * replacing existing ones (names compare without case, as on Windows). Free with compat_strv_free. */
char **compat_env_apply(const char *const *env, const struct compat_hack *hack);
void compat_strv_free(char **strv);

/* New command line with the hack applied, or NULL when it does not apply or is already there. */
char *compat_cmdline_apply(const char *cmdline, const struct compat_hack *hack);

#endif
