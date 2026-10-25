// SPDX-License-Identifier: MIT
// Keep a conservative clock floor on devices whose RTC loses its date.
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr time_t release_epoch = 1792281600; // Fedora 44 image source epoch.
constexpr time_t latest_plausible = 4102444800; // 2100-01-01 UTC.
constexpr char directory[] = "/var/lib/senemos/clock";
constexpr char saved[] = "/var/lib/senemos/clock/last-good";
constexpr char temporary[] = "/var/lib/senemos/clock/last-good.tmp";

time_t read_saved()
{
    int fd = open(saved, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0)
        return 0;
    struct stat st {};
    char text[32] {};
    ssize_t count = read(fd, text, sizeof(text) - 1);
    int error = fstat(fd, &st);
    close(fd);
    if (error || !S_ISREG(st.st_mode) || count <= 0 || count >= 20)
        return 0;
    char *end = nullptr;
    errno = 0;
    long long value = strtoll(text, &end, 10);
    if (errno || end != text + count || value < release_epoch ||
        value > latest_plausible)
        return 0;
    return static_cast<time_t>(value);
}

int save(time_t now)
{
    if (now < release_epoch || now > latest_plausible || now <= read_saved())
        return 0;
    if (mkdir("/var/lib/senemos", 0755) && errno != EEXIST)
        return 1;
    if (mkdir(directory, 0700) && errno != EEXIST)
        return 1;
    int fd = open(temporary, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0)
        return 1;
    char text[32];
    int count = snprintf(text, sizeof(text), "%lld", static_cast<long long>(now));
    bool ok = count > 0 && count < static_cast<int>(sizeof(text)) &&
              write(fd, text, count) == count && fsync(fd) == 0;
    if (close(fd))
        ok = false;
    if (!ok || rename(temporary, saved)) {
        unlink(temporary);
        return 1;
    }
    int dir = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (dir < 0)
        return 1;
    int result = fsync(dir);
    close(dir);
    return result ? 1 : 0;
}
}

int main(int argc, char **argv)
{
    if (argc != 2 || (strcmp(argv[1], "restore") && strcmp(argv[1], "save"))) {
        fprintf(stderr, "Usage: uke-clock-state restore|save\n");
        return 2;
    }
    struct timespec now {};
    if (clock_gettime(CLOCK_REALTIME, &now))
        return 1;
    if (!strcmp(argv[1], "save"))
        return save(now.tv_sec);
    time_t floor = read_saved();
    if (floor < release_epoch)
        floor = release_epoch;
    if (now.tv_sec >= floor)
        return 0;
    struct timespec corrected {floor, 0};
    if (clock_settime(CLOCK_REALTIME, &corrected)) {
        fprintf(stderr, "Cannot restore clock floor: %s\n", strerror(errno));
        return 1;
    }
    fprintf(stderr, "Advanced clock to last known time: %lld UTC\n",
            static_cast<long long>(floor));
    return 0;
}
