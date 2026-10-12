/* Unit tests for src/winhubcompat/compatdb.c. Built natively by scripts/test-compatdb.sh. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "compatdb.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static int warnings;
static void count_warning(const char *format, ...) { (void)format; warnings++; }

static const char *db_text =
    "{ \"version\": 1, \"timestamp\": 100, \"rules\": ["
    "  { \"desc\": \"game\", \"os\": \"macos\","
    "    \"match\": { \"type\": \"path_suffix\", \"suffix\": \"Game/bin/game.exe\" },"
    "    \"hacks\": [ { \"type\": \"graphics_backend\", \"backend\": \"dxmt\" },"
    "                 { \"type\": \"env_vars\", \"vars\": { \"DXMT_LOG_LEVEL\": \"none\", \"Path\": \"C:\\\\x\" } },"
    "                 { \"type\": \"append_cmd_line\", \"append\": \"-dx11\", \"forbidden_args\": \"-dx12\", \"insert\": true } ] },"
    "  { \"desc\": \"steam\", \"match\": { \"type\": \"steam_game_id\", \"game_id\": \"367520\" },"
    "    \"hacks\": [ { \"type\": \"graphics_backend\", \"backend\": \"dxvk\", \"force\": true },"
    "                 { \"type\": \"dll_overrides\", \"overrides\": [ \"d3d9=n,b\" ] } ] },"
    "  { \"desc\": \"linux only\", \"os\": \"linux\", \"match\": { \"type\": \"path_suffix\", \"suffix\": \"a.exe\" },"
    "    \"hacks\": [ { \"type\": \"graphics_backend\", \"backend\": \"dxvk\" } ] },"
    "  { \"desc\": \"bad backend\", \"match\": { \"type\": \"path_suffix\", \"suffix\": \"b.exe\" },"
    "    \"hacks\": [ { \"type\": \"graphics_backend\", \"backend\": \"opengl\" } ] },"
    "  { \"desc\": \"second game rule\", \"match\": { \"type\": \"path_suffix\", \"suffix\": \"game.exe\" },"
    "    \"hacks\": [ { \"type\": \"graphics_backend\", \"backend\": \"wined3d\" },"
    "                 { \"type\": \"dll_overrides\", \"overrides\": [ \"dxgi=b\" ] } ] }"
    "] }";

static void test_parse_and_find(void)
{
    struct compat_db db;
    const struct compat_hack *found[COMPAT_HACK_COUNT];
    const struct compat_rule *rule[COMPAT_HACK_COUNT];

    warnings = 0;
    CHECK(!compat_db_parse(db_text, 1, &db));
    CHECK(db.count == 3);          /* linux-only and bad-backend rules are skipped */
    CHECK(warnings == 2);          /* bad backend and its now-empty rule; other OS is not an error */
    CHECK(db.timestamp == 100);

    compat_db_find(&db, "C:\\Program Files\\Game\\bin\\GAME.exe", NULL, found, rule);
    CHECK(found[COMPAT_HACK_GRAPHICS_BACKEND] && found[COMPAT_HACK_GRAPHICS_BACKEND]->backend == COMPAT_BACKEND_DXMT);
    CHECK(rule[COMPAT_HACK_GRAPHICS_BACKEND] && !strcmp(rule[COMPAT_HACK_GRAPHICS_BACKEND]->desc, "game"));
    CHECK(found[COMPAT_HACK_ENV_VARS] && found[COMPAT_HACK_ENV_VARS]->count == 2);
    /* first hack of each type wins; the second rule still supplies dll_overrides */
    CHECK(found[COMPAT_HACK_DLL_OVERRIDES] && !strcmp(found[COMPAT_HACK_DLL_OVERRIDES]->names[0], "dxgi=b"));

    compat_db_find(&db, "C:\\Other\\notgame.exe", NULL, found, NULL);
    CHECK(!found[COMPAT_HACK_GRAPHICS_BACKEND]);   /* suffix must start at a path component */
    compat_db_find(&db, "C:\\Other\\game.exe", "367520", found, NULL);
    CHECK(found[COMPAT_HACK_GRAPHICS_BACKEND] && found[COMPAT_HACK_GRAPHICS_BACKEND]->backend == COMPAT_BACKEND_DXVK);
    CHECK(found[COMPAT_HACK_DLL_OVERRIDES] && !strcmp(found[COMPAT_HACK_DLL_OVERRIDES]->names[0], "d3d9=n,b"));
    compat_db_free(&db);
}

static void test_invalid_files(void)
{
    struct compat_db db;

    CHECK(compat_db_parse("not json", 1, &db) == -1);
    CHECK(compat_db_parse("{ \"version\": 2, \"timestamp\": 1, \"rules\": [] }", 1, &db) == -1);
    CHECK(compat_db_parse("{ \"version\": 1, \"rules\": [] }", 1, &db) == -1);
    CHECK(!compat_db_parse("{ \"version\": 1, \"rules\": [] }", 0, &db));
    compat_db_free(&db);
    CHECK(compat_db_parse("{ \"version\": 1, \"timestamp\": 1, \"rules\": {} }", 1, &db) == -1);
}

static int usable_set;
static int usable(enum compat_backend backend, void *context)
{
    (void)context;
    return (usable_set >> backend) & 1;
}
#define USABLE(b) (1 << COMPAT_BACKEND_##b)

static void test_choose_backend(void)
{
    struct compat_hack rule = { .type = COMPAT_HACK_GRAPHICS_BACKEND, .backend = COMPAT_BACKEND_DXVK };

    usable_set = USABLE(DXMT) | USABLE(DXVK);
    CHECK(compat_choose_backend(COMPAT_BACKEND_AUTO, NULL, usable, NULL) == COMPAT_BACKEND_DXMT);
    usable_set |= USABLE(D3DMETAL);
    CHECK(compat_choose_backend(COMPAT_BACKEND_AUTO, NULL, usable, NULL) == COMPAT_BACKEND_D3DMETAL);
    CHECK(compat_choose_backend(COMPAT_BACKEND_AUTO, &rule, usable, NULL) == COMPAT_BACKEND_DXVK);
    /* the user's choice beats a rule unless the rule is forced */
    CHECK(compat_choose_backend(COMPAT_BACKEND_DXMT, &rule, usable, NULL) == COMPAT_BACKEND_DXMT);
    rule.force = 1;
    CHECK(compat_choose_backend(COMPAT_BACKEND_DXMT, &rule, usable, NULL) == COMPAT_BACKEND_DXVK);
    CHECK(compat_choose_backend(COMPAT_BACKEND_WINED3D, NULL, usable, NULL) == COMPAT_BACKEND_WINED3D);
    /* unusable choices fall through to the next candidate */
    usable_set = USABLE(DXMT);
    CHECK(compat_choose_backend(COMPAT_BACKEND_D3DMETAL, NULL, usable, NULL) == COMPAT_BACKEND_DXMT);
    usable_set = 0;
    CHECK(compat_choose_backend(COMPAT_BACKEND_DXVK, &rule, usable, NULL) == COMPAT_BACKEND_WINED3D);
}

static void test_env(void)
{
    char *names[] = { "DXMT_LOG_LEVEL", "PATH" }, *values[] = { "none", "C:\\x" };
    struct compat_hack hack = { .type = COMPAT_HACK_ENV_VARS, .count = 2, .names = names, .values = values };
    const char *env[] = { "Path=C:\\windows", "TEMP=C:\\temp", NULL };
    char **result = compat_env_apply(env, &hack);

    CHECK(result && !strcmp(result[0], "TEMP=C:\\temp"));
    CHECK(result && !strcmp(result[1], "DXMT_LOG_LEVEL=none"));
    CHECK(result && !strcmp(result[2], "PATH=C:\\x"));   /* replaced, not duplicated */
    CHECK(result && !result[3]);
    compat_strv_free(result);
}

static void test_cmdline(void)
{
    struct compat_hack hack = { .type = COMPAT_HACK_APPEND_CMD_LINE, .append = "-dx11" };
    char *result;

    result = compat_cmdline_apply("game.exe -windowed", &hack);
    CHECK(result && !strcmp(result, "game.exe -windowed -dx11"));
    free(result);
    hack.insert = 1;
    result = compat_cmdline_apply("\"C:\\Game Dir\\game.exe\" -windowed", &hack);
    CHECK(result && !strcmp(result, "\"C:\\Game Dir\\game.exe\" -dx11 -windowed"));
    free(result);
    CHECK(!compat_cmdline_apply("game.exe -DX11", &hack));        /* already there */
    hack.forbidden_args = "-dx12";
    CHECK(!compat_cmdline_apply("game.exe -dx12", &hack));
    hack.forbidden_args = NULL;
    hack.required_args = "-launcher";
    CHECK(!compat_cmdline_apply("game.exe", &hack));
    result = compat_cmdline_apply("game.exe -launcher", &hack);
    CHECK(result && !strcmp(result, "game.exe -dx11 -launcher"));
    free(result);
}

static void write_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");
    fputs(text, file);
    fclose(file);
}

static void test_load(void)
{
    char dir[] = "/tmp/compatdb-test.XXXXXX", builtin[256], downloaded[256], local[256];
    struct compat_db db;
    const char *rule = "{ \"desc\": \"%s\", \"match\": { \"type\": \"path_suffix\", \"suffix\": \"x.exe\" },"
                       " \"hacks\": [ { \"type\": \"graphics_backend\", \"backend\": \"%s\" } ] }";
    char text[1024], one[512];

    CHECK(mkdtemp(dir) != NULL);
    snprintf(builtin, sizeof(builtin), "%s/builtin.json", dir);
    snprintf(downloaded, sizeof(downloaded), "%s/downloaded.json", dir);
    snprintf(local, sizeof(local), "%s/local.json", dir);

    snprintf(one, sizeof(one), rule, "builtin", "dxmt");
    snprintf(text, sizeof(text), "{ \"version\": 1, \"timestamp\": 200, \"rules\": [ %s ] }", one);
    write_file(builtin, text);
    snprintf(one, sizeof(one), rule, "downloaded", "dxvk");
    snprintf(text, sizeof(text), "{ \"version\": 1, \"timestamp\": 100, \"rules\": [ %s ] }", one);
    write_file(downloaded, text);

    /* an older download loses to the built-in database */
    compat_db_load(builtin, downloaded, local, &db);
    CHECK(db.count == 1 && !strcmp(db.rules[0].desc, "builtin"));
    compat_db_free(&db);

    snprintf(text, sizeof(text), "{ \"version\": 1, \"timestamp\": 300, \"rules\": [ %s ] }", one);
    write_file(downloaded, text);
    snprintf(one, sizeof(one), rule, "local", "wined3d");
    snprintf(text, sizeof(text), "{ \"version\": 1, \"rules\": [ %s ] }", one);
    write_file(local, text);

    /* the user's rules come first, then the newer database */
    compat_db_load(builtin, downloaded, local, &db);
    CHECK(db.count == 2 && !strcmp(db.rules[0].desc, "local") && !strcmp(db.rules[1].desc, "downloaded"));
    CHECK(db.timestamp == 300);
    compat_db_free(&db);

    compat_db_load(NULL, "/nonexistent.json", NULL, &db);
    CHECK(db.count == 0);
    compat_db_free(&db);

    unlink(builtin);
    unlink(downloaded);
    unlink(local);
    rmdir(dir);
}

int main(void)
{
    compat_warn = count_warning;
    test_parse_and_find();
    test_invalid_files();
    test_choose_backend();
    test_env();
    test_cmdline();
    test_load();
    if (failures) fprintf(stderr, "%d failure(s)\n", failures);
    else printf("compatdb: all tests passed\n");
    return failures != 0;
}
