////////////////////////////////////////////////////////////////////////////////
// MailBox
////////////////////////////////////////////////////////////////////////////////

#include "mailbox.h"

#include "actorcell.h"
#include "msg.h"

typedef struct MailBox {
  Queue *queue;
  pthread_mutex_t readLock;
  pthread_mutex_t writeLock;
} MailBox;

MailBox *mailbox_create(int size) {
  MailBox *mailbox = malloc(sizeof(MailBox));
  mailbox->queue = queue_create(size);
  mailbox->readLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  mailbox->writeLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  return mailbox;
}

void mailbox_clear(MailBox *mailbox) {
  pthread_mutex_lock(&mailbox->readLock);
  Msg *msg = NULL;
  while ((msg = queue_get(mailbox->queue))) {
    msg_free(msg);
  }
  pthread_mutex_unlock(&mailbox->readLock);
}

void mailbox_free(MailBox *mailbox) {
  mailbox_clear(mailbox);
  queue_free(mailbox->queue);
  free(mailbox);
}

bool mailbox_is_empty(MailBox *mailbox) {
  pthread_mutex_lock(&mailbox->readLock);
  bool empty = queue_is_empty(mailbox->queue);
  pthread_mutex_unlock(&mailbox->readLock);
  return empty;
}

void mailbox_push(MailBox *mailbox, Msg *msg) {
  pthread_mutex_lock(&mailbox->writeLock);
  queue_add(mailbox->queue, msg);
  pthread_mutex_unlock(&mailbox->writeLock);
}

Msg * mailbox_pull(MailBox *mailbox) {
  pthread_mutex_lock(&mailbox->readLock);
  Msg *msg = queue_get(mailbox->queue);
  pthread_mutex_unlock(&mailbox->readLock);
  return msg;
}
