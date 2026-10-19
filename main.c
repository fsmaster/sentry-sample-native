/*
 * sentry-native sample with the crashpad backend.
 *
 *   ./sample crash <run>   real SIGSEGV, crashpad writes and uploads a minidump
 *   ./sample error <run>   handled error with a stack trace
 */
#include <sentry.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    const char *name;
    int *scores; /* NULL for a player who never played */
} player_t;

static int best_score(const player_t *p, int count)
{
    int best = 0;
    for (int i = 0; i < count; i++) {
        if (p->scores[i] > best) { /* NULL dereference here */
            best = p->scores[i];
        }
    }
    return best;
}

static int leaderboard(const char *name)
{
    player_t player = { name, NULL };
    return best_score(&player, 3);
}

static const char *env_or(const char *key, const char *fallback)
{
    const char *v = getenv(key);
    return v && *v ? v : fallback;
}

int main(int argc, char **argv)
{
    const char *kind = argc > 1 ? argv[1] : "crash";
    const char *run = argc > 2 ? argv[2] : "manual";
    int fresh = argc > 3 ? atoi(argv[3]) : 1;

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, getenv("SENTRY_DSN"));
    sentry_options_set_release(options, env_or("RELEASE", "sample-native@dev"));
    sentry_options_set_environment(options, env_or("SENTRY_ENVIRONMENT", "local"));
    sentry_options_set_handler_path(options, env_or("CRASHPAD_HANDLER", "./crashpad_handler"));
    sentry_options_set_database_path(options, "/tmp/.sentry-native");
    sentry_init(options);

    sentry_set_tag("run_id", run);
    if (fresh) {
        sentry_set_fingerprint("{{ default }}", run, NULL);
    }

    if (kind[0] == 'c') {
        printf("crashing, minidump goes to Sentry...\n");
        fflush(stdout);
        return leaderboard(run);
    }

    sentry_value_t event = sentry_value_new_event();
    sentry_value_t exc = sentry_value_new_exception("SaveError", "could not write save file");
    sentry_value_set_stacktrace(exc, NULL, 0);
    sentry_event_add_exception(event, exc);
    sentry_uuid_t id = sentry_capture_event(event);
    char buf[37];
    sentry_uuid_as_string(&id, buf);
    printf("event id: %s\n", buf);
    sentry_close();
    return 0;
}
