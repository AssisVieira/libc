#ifndef CONTROL_PRODUTO_H
#define CONTROL_PRODUTO_H

#include "db/db.h"

void control_produto_init(DBPool *pool);

void control_produto_add();

void control_produto_list();

void control_produto_edit();

void control_produto_del();

#endif
