#ifndef DB_H
#define DB_H

#include <sqlite3.h>

extern sqlite3 *db;

void db_init();
int db_check_user(const char *username, const char *password);
int db_user_exists(const char *username);
void db_add_user(const char *username, const char *password);

#endif // DB_H
