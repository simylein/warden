const char *device_table = "device";
const char *device_schema = "create table device ("
														"id blob primary key, "
														"name text not null unique"
														")";
