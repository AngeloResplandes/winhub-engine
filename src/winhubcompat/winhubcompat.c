/*
 * winhubcompat: Direct3D backend choice and per-game settings for WinHub's Wine
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

/* ntdll.so loads this library in every Wine process, after mapping ntdll.dll and before any
 * other DLL (patches/wine/0001). Its constructor:
 *  - reads the compatibility rules (docs/compatdb.md);
 *  - applies the matching rule's DLL overrides and environment variables to this process;
 *  - picks the Direct3D backend (WINHUB_GRAPHICS_BACKEND or a rule, falling back when a backend
 *    cannot run here) and puts its DLL folder first in Wine's search path, so nothing is copied
 *    into the bottle;
 *  - wraps NtCreateUserProcess so child processes get the matching rule's environment variables
 *    and command-line additions.
 * It takes the place of the closed cxcompatdb.so that CodeWeavers' ntdll loads in CrossOver. */

#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/unixlib.h"
#include "wine/debug.h"

#include "compatdb.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhubcompat);

typedef NTSTATUS (*create_user_process_func)( HANDLE *, HANDLE *, ACCESS_MASK, ACCESS_MASK,
                                              OBJECT_ATTRIBUTES *, OBJECT_ATTRIBUTES *, ULONG, ULONG,
                                              RTL_USER_PROCESS_PARAMETERS *, PS_CREATE_INFO *,
                                              PS_ATTRIBUTE_LIST * );

static struct compat_db db;
static char *root_dir;  /* the runtime's folder with bin/ and lib/ */
static create_user_process_func real_NtCreateUserProcess;

static void warn_db( const char *format, ... )
{
    char buffer[1024];
    va_list args;

    va_start( args, format );
    vsnprintf( buffer, sizeof(buffer), format, args );
    va_end( args );
    WARN( "%s", buffer );
}

static char *path_join( const char *dir, const char *name )
{
    char *path;
    return asprintf( &path, "%s/%s", dir, name ) < 0 ? NULL : path;
}

/* <root>/lib/wine/x86_64-unix/winhubcompat.so -> <root> */
static char *get_root_dir( void )
{
    Dl_info info;
    char *path, *slash;
    int i;

    if (!dladdr( (void *)get_root_dir, &info ) || !info.dli_fname) return NULL;
    if (!(path = realpath( info.dli_fname, NULL ))) return NULL;
    for (i = 0; i < 4; i++)
    {
        if (!(slash = strrchr( path, '/' )))
        {
            free( path );
            return NULL;
        }
        *slash = 0;
    }
    return path;
}

static char *wide_to_utf8( const WCHAR *str, DWORD len )
{
    char *ret;
    int size;

    if (!str) return NULL;
    if (!(ret = malloc( len * 3 + 1 ))) return NULL;
    size = ntdll_wcstoumbs( str, len, ret, len * 3, FALSE );
    ret[size < 0 ? 0 : size] = 0;
    return ret;
}

static WCHAR *utf8_to_wide( const char *str, DWORD *len_ret )
{
    DWORD len = strlen( str ), count;
    WCHAR *ret = malloc( (len + 1) * sizeof(WCHAR) );

    if (!ret) return NULL;
    count = ntdll_umbstowcs( str, len, ret, len );
    ret[count] = 0;
    if (len_ret) *len_ret = count;
    return ret;
}

/* Windows environment block ("A=1\0B=2\0\0") to NULL-terminated UTF-8 strings. */
static char **env_block_to_strv( const WCHAR *block )
{
    const WCHAR *p;
    unsigned int count = 0, i = 0;
    char **ret;

    if (!block) return calloc( 1, sizeof(char *) );
    for (p = block; *p; p += wcslen( p ) + 1) count++;
    if (!(ret = calloc( count + 1, sizeof(char *) ))) return NULL;
    for (p = block; *p; p += wcslen( p ) + 1)
    {
        if (!(ret[i++] = wide_to_utf8( p, wcslen( p ) )))
        {
            compat_strv_free( ret );
            return NULL;
        }
    }
    return ret;
}

static WCHAR *strv_to_env_block( char **strv, SIZE_T *size_ret )
{
    SIZE_T total = 1, pos = 0;
    unsigned int i;
    WCHAR *block;

    for (i = 0; strv[i]; i++) total += strlen( strv[i] ) + 1;
    if (!(block = malloc( total * sizeof(WCHAR) ))) return NULL;
    for (i = 0; strv[i]; i++)
    {
        DWORD len = strlen( strv[i] );
        pos += ntdll_umbstowcs( strv[i], len, block + pos, total - pos );
        block[pos++] = 0;
    }
    block[pos++] = 0;
    *size_ret = pos * sizeof(WCHAR);
    return block;
}

static const char *strv_lookup( char **strv, const char *name )
{
    size_t len = strlen( name );
    unsigned int i;

    for (i = 0; strv && strv[i]; i++)
        if (!strncasecmp( strv[i], name, len ) && strv[i][len] == '=') return strv[i] + len + 1;
    return NULL;
}

static const char *rule_name( const struct compat_rule *rule )
{
    return rule && rule->desc ? rule->desc : "<no description>";
}


/* where each backend can run */

static int is_translated( void )
{
    static int translated = -1;

    if (translated == -1)
    {
        int value = 0;
        size_t size = sizeof(value);
        translated = !sysctlbyname( "sysctl.proc_translated", &value, &size, NULL, 0 ) && value == 1;
    }
    return translated;
}

static int is_sonoma_or_later( void )
{
    char version[32];
    size_t size = sizeof(version);

    if (sysctlbyname( "kern.osproductversion", version, &size, NULL, 0 )) return 0;
    return atoi( version ) >= 14;
}

static int is_wow64( void )
{
    ULONG_PTR peb32 = 0;

    if (NtQueryInformationProcess( NtCurrentProcess(), ProcessWow64Information, &peb32, sizeof(peb32), NULL ))
        return 0;
    return peb32 != 0;
}

/* Folder holding the backend's x86_64-windows/, i386-windows/ and x86_64-unix/ DLLs. */
static char *backend_dll_dir( enum compat_backend backend )
{
    const char *dir;

    switch (backend)
    {
    case COMPAT_BACKEND_DXMT:
        return root_dir ? path_join( root_dir, "lib/dxmt" ) : NULL;
    case COMPAT_BACKEND_DXVK:
        return root_dir ? path_join( root_dir, "lib/dxvk" ) : NULL;
    case COMPAT_BACKEND_D3DMETAL:
        /* Apple's Game Porting Toolkit, installed by the user; WinHub never ships it. */
        dir = getenv( "WINHUB_D3DMETAL_DIR" );
        return dir && *dir ? path_join( dir, "wine" ) : NULL;
    default:
        return NULL;
    }
}

static int backend_usable( enum compat_backend backend, void *context )
{
    char *dir, *dll;
    int present;

    switch (backend)
    {
    case COMPAT_BACKEND_WINED3D:
        return 1;
    case COMPAT_BACKEND_DXMT:
        if (!is_translated() || !is_sonoma_or_later()) return 0;
        break;
    case COMPAT_BACKEND_D3DMETAL:
        if (!is_translated() || !is_sonoma_or_later() || is_wow64()) return 0;
        break;
    case COMPAT_BACKEND_DXVK:
        /* Without VK_EXT_map_memory_placed, 32-bit processes get buffers backed by host allocations
         * (win32u_vkCreateBuffer), which MoltenVK refuses; CrossOver's MoltenVK refuses them too. */
        if (is_wow64()) return 0;
        break;
    default:
        return 0;
    }

    if (!(dir = backend_dll_dir( backend ))) return 0;
    dll = path_join( dir, "x86_64-windows/d3d11.dll" );
    present = dll && !access( dll, F_OK );
    if (!present) TRACE( "%s is not installed (%s)\n", compat_backend_name( backend ), debugstr_a(dll) );
    free( dll );
    free( dir );
    return present;
}


/* this process */

static void set_graphics_backend( const struct compat_hack *hack, const struct compat_rule *rule )
{
    const char *setting = getenv( "WINHUB_GRAPHICS_BACKEND" );
    void (*prepend_dll_path)( const char * );
    int user = compat_backend_from_name( setting );
    enum compat_backend backend;
    char *dir;

    if (user < 0)
    {
        WARN( "unknown WINHUB_GRAPHICS_BACKEND %s, using auto\n", debugstr_a(setting) );
        user = COMPAT_BACKEND_AUTO;
    }
    if (hack)
        TRACE( "rule %s: graphics backend %s%s\n", rule_name( rule ), compat_backend_name( hack->backend ),
               hack->force ? " (forced)" : "" );

    backend = compat_choose_backend( user, hack, backend_usable, NULL );
    if (user != COMPAT_BACKEND_AUTO && backend != (enum compat_backend)user && !(hack && hack->force))
        TRACE( "%s cannot be used here\n", compat_backend_name( user ) );
    TRACE( "using %s as the graphics backend\n", compat_backend_name( backend ) );

    /* CX_ACTIVE_GRAPHICS_BACKEND is read by CodeWeavers' code in win32u. */
    setenv( "CX_ACTIVE_GRAPHICS_BACKEND", compat_backend_name( backend ), 1 );
    setenv( "WINHUB_ACTIVE_GRAPHICS_BACKEND", compat_backend_name( backend ), 1 );

    if (backend == COMPAT_BACKEND_WINED3D || !(dir = backend_dll_dir( backend ))) return;
    if (!(prepend_dll_path = dlsym( RTLD_DEFAULT, "prepend_dll_path" )))
    {
        WARN( "ntdll does not export prepend_dll_path\n" );
        free( dir );
        return;
    }
    prepend_dll_path( dir );  /* keeps the string */
}

static void add_dll_overrides( const struct compat_hack *hack, const struct compat_rule *rule )
{
    void (*add_load_order_override)( const WCHAR * ) = dlsym( RTLD_DEFAULT, "add_load_order_override" );
    unsigned int i;

    if (!add_load_order_override)
    {
        WARN( "ntdll does not export add_load_order_override\n" );
        return;
    }
    TRACE( "rule %s: adding %u dll overrides\n", rule_name( rule ), hack->count );
    for (i = 0; i < hack->count; i++)
    {
        WCHAR *entry = utf8_to_wide( hack->names[i], NULL );
        if (!entry) continue;
        add_load_order_override( entry );  /* copies it */
        free( entry );
    }
}

static void apply_in_process( void )
{
    const struct compat_hack *found[COMPAT_HACK_COUNT];
    const struct compat_rule *rule[COMPAT_HACK_COUNT];
    RTL_USER_PROCESS_PARAMETERS *params = NtCurrentTeb()->Peb->ProcessParameters;
    const char *steam_game_id = getenv( "SteamGameId" );
    char *image = NULL;
    unsigned int i;

    if (params && params->ImagePathName.Buffer)
        image = wide_to_utf8( params->ImagePathName.Buffer, params->ImagePathName.Length / sizeof(WCHAR) );
    TRACE( "program %s, SteamGameId %s\n", debugstr_a(image), debugstr_a(steam_game_id) );
    compat_db_find( &db, image, steam_game_id, found, rule );

    if (found[COMPAT_HACK_DLL_OVERRIDES]) add_dll_overrides( found[COMPAT_HACK_DLL_OVERRIDES], rule[COMPAT_HACK_DLL_OVERRIDES] );
    if (found[COMPAT_HACK_ENV_VARS])
    {
        /* Only the Unix side of this process sees these (Metal, MoltenVK, DXMT's Unix library);
         * child processes get them in their Windows environment too. */
        TRACE( "rule %s: setting %u env vars\n", rule_name( rule[COMPAT_HACK_ENV_VARS] ), found[COMPAT_HACK_ENV_VARS]->count );
        for (i = 0; i < found[COMPAT_HACK_ENV_VARS]->count; i++)
            setenv( found[COMPAT_HACK_ENV_VARS]->names[i], found[COMPAT_HACK_ENV_VARS]->values[i], 1 );
    }
    set_graphics_backend( found[COMPAT_HACK_GRAPHICS_BACKEND], rule[COMPAT_HACK_GRAPHICS_BACKEND] );
    free( image );
}


/* child processes */

static NTSTATUS hooked_NtCreateUserProcess( HANDLE *process_handle, HANDLE *thread_handle,
                                            ACCESS_MASK process_access, ACCESS_MASK thread_access,
                                            OBJECT_ATTRIBUTES *process_attr, OBJECT_ATTRIBUTES *thread_attr,
                                            ULONG process_flags, ULONG thread_flags,
                                            RTL_USER_PROCESS_PARAMETERS *params, PS_CREATE_INFO *info,
                                            PS_ATTRIBUTE_LIST *ps_attr )
{
    const struct compat_hack *found[COMPAT_HACK_COUNT];
    const struct compat_rule *rule[COMPAT_HACK_COUNT];
    RTL_USER_PROCESS_PARAMETERS copy;
    char *image = NULL, *cmdline = NULL, *new_cmdline = NULL;
    char **env = NULL, **new_env = NULL;
    WCHAR *env_block = NULL, *cmdline_w = NULL;
    NTSTATUS status;

    /* Normalized parameters hold plain pointers, so a copy can point at new strings. */
    if (!params || !(params->Flags & PROCESS_PARAMS_FLAG_NORMALIZED))
        return real_NtCreateUserProcess( process_handle, thread_handle, process_access, thread_access,
                                         process_attr, thread_attr, process_flags, thread_flags,
                                         params, info, ps_attr );

    copy = *params;
    image = wide_to_utf8( params->ImagePathName.Buffer, params->ImagePathName.Length / sizeof(WCHAR) );
    env = env_block_to_strv( params->Environment );
    compat_db_find( &db, image, strv_lookup( env, "SteamGameId" ), found, rule );

    if (found[COMPAT_HACK_ENV_VARS] && env &&
        (new_env = compat_env_apply( (const char *const *)env, found[COMPAT_HACK_ENV_VARS] )) &&
        (env_block = strv_to_env_block( new_env, &copy.EnvironmentSize )))
    {
        TRACE( "rule %s: setting %u env vars for %s\n", rule_name( rule[COMPAT_HACK_ENV_VARS] ),
               found[COMPAT_HACK_ENV_VARS]->count, debugstr_a(image) );
        copy.Environment = env_block;
    }

    if (found[COMPAT_HACK_APPEND_CMD_LINE] && params->CommandLine.Buffer)
    {
        cmdline = wide_to_utf8( params->CommandLine.Buffer, params->CommandLine.Length / sizeof(WCHAR) );
        if (cmdline && (new_cmdline = compat_cmdline_apply( cmdline, found[COMPAT_HACK_APPEND_CMD_LINE] )))
        {
            DWORD len;
            if ((cmdline_w = utf8_to_wide( new_cmdline, &len )) && len < 0x7fff)
            {
                TRACE( "rule %s: command line %s\n", rule_name( rule[COMPAT_HACK_APPEND_CMD_LINE] ),
                       debugstr_a(new_cmdline) );
                copy.CommandLine.Buffer = cmdline_w;
                copy.CommandLine.Length = len * sizeof(WCHAR);
                copy.CommandLine.MaximumLength = (len + 1) * sizeof(WCHAR);
            }
        }
    }

    status = real_NtCreateUserProcess( process_handle, thread_handle, process_access, thread_access,
                                       process_attr, thread_attr, process_flags, thread_flags,
                                       &copy, info, ps_attr );
    free( cmdline_w );
    free( new_cmdline );
    free( cmdline );
    free( env_block );
    compat_strv_free( new_env );
    compat_strv_free( env );
    free( image );
    return status;
}

/* Point the NtCreateUserProcess entry of ntdll's syscall table at the wrapper. */
static void hook_create_user_process( void )
{
    SYSTEM_SERVICE_TABLE *table = dlsym( RTLD_DEFAULT, "KeServiceDescriptorTable" );
    ULONG_PTR i, page = getpagesize();

    if (!table || !table->ServiceTable)
    {
        WARN( "ntdll does not export KeServiceDescriptorTable\n" );
        return;
    }
    for (i = 0; i < table->ServiceLimit; i++)
    {
        void *start;

        if (table->ServiceTable[i] != (ULONG_PTR)NtCreateUserProcess) continue;
        start = (void *)((ULONG_PTR)&table->ServiceTable[i] & ~(page - 1));
        /* The table stays writable afterwards: restoring a guessed protection could also take
         * write access away from unrelated data on the same page. */
        if (mprotect( start, page, PROT_READ | PROT_WRITE ))
        {
            WARN( "cannot make the syscall table writable: %d\n", errno );
            return;
        }
        real_NtCreateUserProcess = (create_user_process_func)table->ServiceTable[i];
        table->ServiceTable[i] = (ULONG_PTR)hooked_NtCreateUserProcess;
        return;
    }
    WARN( "NtCreateUserProcess is not in the syscall table\n" );
}

static void __attribute__((constructor)) winhubcompat_init( void )
{
    /* WINHUB_HOME: WinHub's data folder, with the downloaded database (already verified by WinHub).
     * WINHUB_COMPAT_RULES: the bottle's own rules (e.g. a backend chosen for one game). */
    const char *home = getenv( "WINHUB_HOME" ), *local = getenv( "WINHUB_COMPAT_RULES" );
    char *builtin = NULL, *downloaded = NULL;

    compat_warn = warn_db;
    if ((root_dir = get_root_dir())) builtin = path_join( root_dir, "share/winhub-engine/compatdb.json" );
    if (home && *home) downloaded = path_join( home, "compat/compatdb.json" );
    compat_db_load( builtin, downloaded, local && *local ? local : NULL, &db );
    TRACE( "runtime %s, %u rules (database timestamp %.0f)\n", debugstr_a(root_dir), db.count, db.timestamp );
    free( builtin );
    free( downloaded );

    apply_in_process();
    if (db.count) hook_create_user_process();
}
