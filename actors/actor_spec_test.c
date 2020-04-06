#include "actor_spec.h"

#include "asserts/asserts.h"

ACTOR(Pinger, { int max; });

MESSAGE(Create, {});

MESSAGE(Ping, {});

static Msg *pinger_on_create(const Msg *msg) { return NULL; }

static Msg *pinger_on_ping(const Msg *msg) { return NULL; }

ACTOR_IMPL(Pinger,
           {
             int max;
             int count;
           },
           {
               {&Create, pinger_on_create},
               {&Ping, pinger_on_ping},
           });

ACTOR_IMPL(PingerWithoutRouters,
           {
             int max;
             int count;
           },
           {});

int main() {
  assertEqual(Pinger.id, 0);
  assertStrEqual(Pinger.name, "Pinger");
  assertEqual(Pinger.contextSize, sizeof(PingerContext));
  assertPtrEqual(Pinger.routes[0].metaMsg, &Create);
  assertPtrEqual(Pinger.routes[0].handler, pinger_on_create);
  assertPtrEqual(Pinger.routes[1].metaMsg, &Ping);
  assertPtrEqual(Pinger.routes[1].handler, pinger_on_ping);
  assertPtrEqual(Pinger.routes[2].metaMsg, NULL);
  assertPtrEqual(Pinger.routes[2].handler, NULL);
  assertEqual(Pinger.registred, false);

  assertPtrEqual(PingerWithoutRouters.routes[0].metaMsg, NULL);
  assertPtrEqual(PingerWithoutRouters.routes[0].handler, NULL);

  return 0;
}