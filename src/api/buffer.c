const char *buffer_table = "buffer";
const char *buffer_schema = "create table buffer ("
														"id blob primary key, "
														"delay integer not null, "
														"level integer not null, "
														"captured_at timestamp not null, "
														"uplink_id blob not null unique, "
														"device_id blob not null, "
														"foreign key (uplink_id) references uplink(id) on delete cascade, "
														"foreign key (device_id) references device(id) on delete cascade"
														")";
