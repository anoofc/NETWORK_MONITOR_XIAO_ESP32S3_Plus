#include "Model.h"
#include <assert.h>
#include <limits.h>
int main()
{
    PingStats p;
    assert(p.samples.count == 0 && p.successes() == 0 && p.average() == 0);
    p.samples.push({true, 10, 0});
    p.samples.push({false, 0, 0});
    p.samples.push({true, 30, 0});
    assert(p.average() == 20 && p.loss() == 33);
    p.samples.clear();
    for (unsigned i = 0; i < 31; i++)
        p.samples.push({false, 0, i});
    assert(p.samples.count == 30 && p.loss() == 100 && p.average() == 0 && p.samples.oldest(0).at == 1);
    Ring<Event, 16> events;
    for (unsigned i = 0; i < 20; i++)
        events.push({EventKind::Connected, i});
    assert(events.count == 16 && events.oldest(0).at == 4 && events.newest().at == 19);
    ConnectionPolicy c;
    assert(c.attemptDue(0));
    c.start(100, false);
    assert(!c.timedOut(15099) && c.timedOut(15100));
    c.fail(15100);
    assert(!c.attemptDue(16099) && c.attemptDue(16100));
    c.start(16100, true);
    assert(c.state == Link::Reconnecting);
    c.connected(17000);
    assert(c.state == Link::Connected && c.backoff == 1000);
    for (int i = 0; i < 20; i++)
        c.increaseBackoff();
    assert(c.backoff == 30000);
    assert(elapsed(20, UINT32_MAX - 20, 41));
    assert(!elapsed(20, UINT32_MAX - 20, 42));
    char ssid[40];
    asciiSSID(ssid, sizeof ssid, "caf\xc3\xa9");
    assert(!strcmp(ssid, "caf?"));
    puts("All model checks passed");
}
