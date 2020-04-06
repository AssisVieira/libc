#include "msg_spec.h"

#include "asserts/asserts.h"

int main() {
  MESSAGE(CreateUser, {
    char name[128];
    char email[128];
    char phone[128];
  });

  assertEqual(CreateUser.id, 0);
  assertStrEqual(CreateUser.name, "CreateUser");
  assertEqual(CreateUser.paramsSize, sizeof(CreateUserParams));
  assertEqual(CreateUser.registred, false);

  assertEqual(meta_msg_register(&CreateUser), 0);

  assertEqual(CreateUser.id, 0);
  assertStrEqual(CreateUser.name, "CreateUser");
  assertEqual(CreateUser.paramsSize, sizeof(CreateUserParams));
  assertEqual(CreateUser.registred, true);

  return 0;
}