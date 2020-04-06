#include "msg.h"

#include "asserts/asserts.h"

int main() {
  ActorId actorA = 1;
  ActorId actorB = 2;

  MESSAGE(CreateUser, {
    char name[128];
    char email[128];
  });

  Msg *msg = msg_create(actorB, &CreateUser,
                        &(CreateUserParams){
                            .name = "Fulano",
                            .email = "fulano@fulano.com",
                        });

  assertNotNull(msg);
  assertEqual(msg->to, 2);
  assertNotNull(msg->params);
  assertNotNull(msg->meta);

  CreateUserParams *params = (CreateUserParams *)msg->params;
  assertStrEqual(params->name, "Fulano");
  assertStrEqual(params->email, "fulano@fulano.com");

  msg_destroy(msg);

  return 0;
}