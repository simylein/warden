const char *user_device_table = "user device";
const char *user_device_schema = "create table user_device ("
																 "user_id blob not null, "
																 "device_id blob not null, "
																 "primary key (user_id, device_id), "
																 "foreign key (user_id) references user(id) on delete cascade, "
																 "foreign key (device_id) references device(id) on delete cascade"
																 ")";
