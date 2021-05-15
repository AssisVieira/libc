#ifndef MAILBOX_INCLUDE_H
#define MAILBOX_INCLUDE_H

#include "foundation/foundation.h"

typedef struct ActorCell ActorCell;
typedef struct Queue Queue;
typedef struct MailBox MailBox;
typedef struct Msg Msg;

MailBox *mailbox_create(int size);
void mailbox_clear(MailBox *mailbox);
void mailbox_free(MailBox *mailbox);
bool mailbox_is_empty(MailBox *mailbox);
void mailbox_push(MailBox *mailbox, Msg *msg);
Msg *mailbox_pull(MailBox *mailbox);
bool mailbox_process(MailBox *mailbox);

#endif
