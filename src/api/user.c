const char *user_table = "user";
const char *user_schema = "create table user ("
													"id blob primary key, "
													"username text not null unique, "
													"password blob not null"
													")";
